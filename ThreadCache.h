#ifndef THREADCACHE_H
#define THREADCACHE_H
#include<iostream>
#include<type_traits>
#include<vector>
#include "Common.h"
/********************************************
*  创建时间： 2025 / 05 / 16
*  名    称： 高性能内存池
*  版    本： 1.0 单线程版本
*  @author    Hu Mingrui
*  说    明： 本内存池属于第一个单线程版本 主
*  要功能是为了给频繁请求和释放内存的线程提供
*  更高效的操作 主要原理是缓存了内存 且对申请
*  的块按一定规则进行内存对齐 减少了内存外碎片
*  的产生 后续本项目会持续升级更新
* *******************************************
*  修改时间： 2025 / 05 / 23
*  版    本： 1.1 单线程版本 
*  改动说明： 
********************************************/
template <class T>
class MemoryPool {
public:
	MemoryPool() {
		for (size_t i = 0; i < FREELISTSIZE; i++) {
			MR_MemPoolToolKits::_FreeLists freelists;
			_freelists[i] = freelists;
		}
		_remain = 0;
		_memstart = nullptr;
	}

	T* Allocate() {
		// 超出单次的最大申请内存阈值
		constexpr size_t mem_size = MR_MemPoolToolKits::GetSize<T>();
		if (mem_size > MAXSIZE)
			return (T*)malloc(mem_size);

		// 获取实际对齐位数以及预期分配的空间大小
		size_t algin = MR_MemPoolToolKits::SizeClass<mem_size>::_GetAlginNum();
		size_t chunk_size = MR_MemPoolToolKits::SizeClass<mem_size>::_RoundUp();
		size_t pos = get_pos();

		if (_freelists[pos].empty()) {
			// 从chunk_alloc里面拿一大块内存 同时补充_free_lists
			// 这里后续可以考虑采用满反馈调节算法
			size_t num = 128;
			char* chunk = chunk_alloc(chunk_size, num, algin);
			for (size_t i = 0; i < num; i++) {
				_freelists[pos].headpush((T*)chunk);
				chunk += chunk_size;
			}
		}
		return (T*)_freelists[pos].headpop();
	}

	void DeAllocate(T* obj) {
		obj->~T();
		size_t pos = get_pos();
		_freelists[pos].headpush(obj);
	}

	~MemoryPool() {
		for (auto mem : _startrecord)
			free(mem);
	}

private:

	MR_MemPoolToolKits::_FreeLists _freelists[FREELISTSIZE];
	char* _memstart;
	size_t _remain;
	// 记录分配的大块内存起始地址 用于释放
	std::vector<char*> _startrecord;

	size_t get_pos() const{
		constexpr size_t mem_size = MR_MemPoolToolKits::GetSize<T>();
		return MR_MemPoolToolKits::SizeClass<mem_size>::_GetIndex();
	}

	// 重要的块分配函数
	// 后续会改为PAGE_MEM 且加入慢反馈调节算法
	// n是请求的单个块大小 
	char* chunk_alloc(size_t n, size_t& num,size_t algin) {
		char* res = nullptr;
		size_t total_chunk = n * num;
		size_t pos = get_pos();
		// 剩下的内存完美满足要求
		if (_remain >= num * n) {
			res = _memstart;
			_memstart += total_chunk;
			_remain -= total_chunk;
			return res;
		}
		// 剩下的内存能满足一个块 但无法满足多个块
		// 此时返回给上一层调用 且更正num的大小
		else if (_remain >= n) {
			num = _remain / n;
			_remain -= num * n;
			res = _memstart;
			_memstart += num * n;
			return res;
		}
		// 一个块也无法满足了 
		else {
			size_t chunk = 2 * total_chunk;
			// 先榨干剩余价值 将剩下的零碎空间挂载至链表上
			while (_remain > 0) {
				int m = _remain - algin;
				if (m < 0) {
					// 这部分空间可能还有剩余 但目前先不做处理
					break;
				}
				_remain -= algin;
				_freelists[pos].headpush(_memstart);
				_memstart += algin;
			}
			_remain = 0;
			_memstart = (char*)malloc(chunk);
			// 如果连系统都无法分配了 那就去freelists取可能可用的
			if (!_memstart) {
				// 挪动后续的freelists
				for (size_t i = pos + 1; i < FREELISTSIZE; i++) {
					if (!_freelists[i].empty()) {
						_memstart = (char*)_freelists[i].headpop();
						_remain += MR_MemPoolToolKits::GetIndexSize(i);
						// 进入alloc再次去修正num
						return chunk_alloc(n, num,algin);
					}
				}
				// 如果到这里了 说明malloc也不行 freelist也没有剩余满足要求的内存了
				// 只能尝试先清理缓存或模拟OOM机制
				throw std::bad_alloc();
			}
			// 记录给指针分配的地址 也即指针本身的地址
			_startrecord.push_back(_memstart);
			_remain += chunk;
			// 此时应该获得了够多的chunk了 但需要返回一个合适的num用于freelists
			return chunk_alloc(n, num,algin);
		}
	}// chunk_alloc 
};

#endif // ! THREADCHACHE_H


