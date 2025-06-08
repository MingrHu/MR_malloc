#ifndef THREADCACHE_H
#define THREADCACHE_H
#include<iostream>
#include<type_traits>
#include<vector>
#include "Common.h"
/********************************************
*  ����ʱ�䣺 2025 / 05 / 16
*  ��    �ƣ� �������ڴ��
*  ��    ���� 1.0 ���̰߳汾
*  @author    Hu Mingrui
*  ˵    ���� ���ڴ�����ڵ�һ�����̰߳汾 ��
*  Ҫ������Ϊ�˸�Ƶ��������ͷ��ڴ���߳��ṩ
*  ����Ч�Ĳ��� ��Ҫԭ���ǻ������ڴ� �Ҷ�����
*  �Ŀ鰴һ����������ڴ���� �������ڴ�����Ƭ
*  �Ĳ��� ��������Ŀ�������������
* *******************************************
*  �޸�ʱ�䣺 2025 / 05 / 23
*  ��    ���� 1.1 ���̰߳汾 
*  �Ķ�˵���� 
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
		// �������ε���������ڴ���ֵ
		constexpr size_t mem_size = MR_MemPoolToolKits::GetSize<T>();
		if (mem_size > MAXSIZE)
			return (T*)malloc(mem_size);

		// ��ȡʵ�ʶ���λ���Լ�Ԥ�ڷ���Ŀռ��С
		size_t algin = MR_MemPoolToolKits::SizeClass<mem_size>::_GetAlginNum();
		size_t chunk_size = MR_MemPoolToolKits::SizeClass<mem_size>::_RoundUp();
		size_t pos = get_pos();

		if (_freelists[pos].empty()) {
			// ��chunk_alloc������һ����ڴ� ͬʱ����_free_lists
			// ����������Կ��ǲ��������������㷨
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
	// ��¼����Ĵ���ڴ���ʼ��ַ �����ͷ�
	std::vector<char*> _startrecord;

	size_t get_pos() const{
		constexpr size_t mem_size = MR_MemPoolToolKits::GetSize<T>();
		return MR_MemPoolToolKits::SizeClass<mem_size>::_GetIndex();
	}

	// ��Ҫ�Ŀ���亯��
	// �������ΪPAGE_MEM �Ҽ��������������㷨
	// n������ĵ������С 
	char* chunk_alloc(size_t n, size_t& num,size_t algin) {
		char* res = nullptr;
		size_t total_chunk = n * num;
		size_t pos = get_pos();
		// ʣ�µ��ڴ���������Ҫ��
		if (_remain >= num * n) {
			res = _memstart;
			_memstart += total_chunk;
			_remain -= total_chunk;
			return res;
		}
		// ʣ�µ��ڴ�������һ���� ���޷���������
		// ��ʱ���ظ���һ����� �Ҹ���num�Ĵ�С
		else if (_remain >= n) {
			num = _remain / n;
			_remain -= num * n;
			res = _memstart;
			_memstart += num * n;
			return res;
		}
		// һ����Ҳ�޷������� 
		else {
			size_t chunk = 2 * total_chunk;
			// ��ե��ʣ���ֵ ��ʣ�µ�����ռ������������
			while (_remain > 0) {
				int m = _remain - algin;
				if (m < 0) {
					// �ⲿ�ֿռ���ܻ���ʣ�� ��Ŀǰ�Ȳ�������
					break;
				}
				_remain -= algin;
				_freelists[pos].headpush(_memstart);
				_memstart += algin;
			}
			_remain = 0;
			_memstart = (char*)malloc(chunk);
			// �����ϵͳ���޷������� �Ǿ�ȥfreelistsȡ���ܿ��õ�
			if (!_memstart) {
				// Ų��������freelists
				for (size_t i = pos + 1; i < FREELISTSIZE; i++) {
					if (!_freelists[i].empty()) {
						_memstart = (char*)_freelists[i].headpop();
						_remain += MR_MemPoolToolKits::GetIndexSize(i);
						// ����alloc�ٴ�ȥ����num
						return chunk_alloc(n, num,algin);
					}
				}
				// ����������� ˵��mallocҲ���� freelistҲû��ʣ������Ҫ����ڴ���
				// ֻ�ܳ������������ģ��OOM����
				throw std::bad_alloc();
			}
			// ��¼��ָ�����ĵ�ַ Ҳ��ָ�뱾��ĵ�ַ
			_startrecord.push_back(_memstart);
			_remain += chunk;
			// ��ʱӦ�û���˹����chunk�� ����Ҫ����һ�����ʵ�num����freelists
			return chunk_alloc(n, num,algin);
		}
	}// chunk_alloc 
};

#endif // ! THREADCHACHE_H


