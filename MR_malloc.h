#ifndef MR_MALLOC
#define MR_MACLLOC
#include<iostream>
#include<type_traits>
#include<vector>
#define FREELISTSIZE 16 // 自由链表的列表个数
#define CHUNKNUM 128
#define CHUNKSIZE 8  // 最小的块大小
#define MAXSIZE 128 // 单次申请的内存大小阈值
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
*  ！模块函数：
*
********************************************/
namespace MR_MemPoolKits{

	template <class T>
	class MemoryPool {
	public:
		MemoryPool() {
			for (int i = 0; i < FREELISTSIZE; i++)
				_freelists[i] = nullptr;
			_remain = 0;
			_memstart = nullptr;
		}

		T* Allocate(size_t mem_size) {
			if (mem_size > MAXSIZE)
				return (T*)malloc(mem_size);

			size_t pos = get_pos(mem_size);
			if (_freelists[pos] == nullptr) {
				// 从chunk_alloc里面拿一大块内存 同时补充_free_lists
				size_t chunk_size = CHUNKSIZE * (pos + 1);
				size_t num = 128;
				char* chunk = chunk_alloc(chunk_size, num);
				for (int i = 0; i < num; i++) {
					headpush((T*)chunk);
					chunk += chunk_size;
				}
			}
			return (T*)headpop(pos);
		}

		void DeAllocate(T* obj) {
			headpush(obj);
		}

		~MemoryPool() {
			for (auto mem : _startrecord)
				free(mem);
		}

	private:

		void* _freelists[FREELISTSIZE];
		char* _memstart;
		size_t _remain;
		// 记录分配的大块内存起始地址 用于释放
		std::vector<char*> _startrecord;

		void headpush(T* obj) {

			obj->~T();
			int pos = get_pos(sizeof(T));
			// 无论是二级还是一级指针本质上就是用
			// 指针存放某个地址 二级指针解引用是内存块的地址
			*(void**)obj = _freelists[pos];
			_freelists[pos] = obj;
		}

		size_t get_pos(size_t n) const{
			return (n - 1) / CHUNKSIZE;
		}

		void* headpop(size_t pos) {
			// 取二级指针存放的地址
			void* next = *(void**)_freelists[pos];
			// 取二级指针的地址 也就是之前回收可用内存obj地址
			void* res = _freelists[pos];
			_freelists[pos] = next;
			return res;
		}

		char* chunk_alloc(size_t n, size_t& num) {
			char* res = nullptr;
			size_t total_chunk = n * num;
			if (_remain >= num * n) {
				res = _memstart;
				_memstart += total_chunk;
				_remain -= total_chunk;
				return res;
			}
			else if (_remain >= n) {
				num = _remain / n;
				_remain -= num * n;
				res = _memstart;
				_memstart += num * n;
				return res;
			}
			else {
				size_t chunk = 2 * total_chunk;
				// 榨干剩余价值
				while (_remain > 0) {
					int m = _remain / CHUNKSIZE - 1;
					if (m < 0)break;
					_remain -= CHUNKSIZE * (m + 1);
					*(void**)_memstart = _freelists[m];
					_freelists[m] = _memstart;
				}
				_remain = 0;
				_memstart =(char*)malloc(chunk);
				// 如果连系统都无法分配了 那就去freelist取可能可用的
				if (!_memstart) {
					// 挪动后续的freelists
					for (size_t i = get_pos(n) + 1; i < FREELISTSIZE; i++) {
						if (!_freelists[i]) {
							_memstart = (char*)headpop(i);
							_remain += (i + 1) * CHUNKSIZE;
							// 进入alloc再次去修正num
							return chunk_alloc(n, num);
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
				return chunk_alloc(n, num);
			}
		}
	};
};

#endif // !MR_MALLOC


