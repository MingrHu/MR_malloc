#ifndef MR_MALLOC_H
#define MR_MACLLOC_H
#include<iostream>
#include<type_traits>
#include<vector>
#include "Common.h"
#define FREELISTSIZE 104 // ����������б����
#define CHUNKNUM 512
#define MAXSIZE 256 * 1024 // ����������ڴ��С��ֵ
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
		for (int i = 0; i < FREELISTSIZE; i++)
			_freelists[i] = nullptr;
		_remain = 0;
		_memstart = nullptr;
	}

	T* Allocate() {
		// �������ε���������ڴ���ֵ
		constexpr size_t mem_size = MR_MemPoolToolKits::GetSize<T>();
		if (mem_size > MAXSIZE)
			return (T*)malloc(mem_size);

		size_t algin = MR_MemPoolToolKits::SizeClass<mem_size>::_GetAlginNum();
		size_t pos = get_pos();

		if (_freelists[pos] == nullptr) {
			// ��chunk_alloc������һ����ڴ� ͬʱ����_free_lists
			size_t chunk_size = algin * (pos + 1);
			size_t num = 128;
			char* chunk = chunk_alloc(chunk_size, num, algin);
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
	// ��¼����Ĵ���ڴ���ʼ��ַ �����ͷ�
	std::vector<char*> _startrecord;

	void headpush(T* obj) {

		obj->~T();
		int pos = get_pos();
		// �����Ƕ�������һ��ָ�뱾���Ͼ�����
		// ָ����ĳ����ַ ����ָ����������ڴ��ĵ�ַ
		*(void**)obj = _freelists[pos];
		_freelists[pos] = obj;
	}

	size_t get_pos() const{
		constexpr size_t mem_size = MR_MemPoolToolKits::GetSize<T>();
		return MR_MemPoolToolKits::SizeClass<mem_size>::_GetIndex();
	}

	void* headpop(size_t pos) {
		// ȡ����ָ���ŵĵ�ַ
		void* next = *(void**)_freelists[pos];
		// ȡ����ָ��ĵ�ַ Ҳ����֮ǰ���տ����ڴ�obj��ַ
		void* res = _freelists[pos];
		_freelists[pos] = next;
		return res;
	}

	// ��Ҫ�Ŀ���亯��
	char* chunk_alloc(size_t n, size_t& num,size_t algin) {
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
			// ե��ʣ���ֵ
			while (_remain > 0) {
				int m = _remain / algin - 1;
				if (m < 0)break;
				_remain -= algin * (m + 1);
				*(void**)_memstart = _freelists[m];
				_freelists[m] = _memstart;
			}
			_remain = 0;
			_memstart = (char*)malloc(chunk);
			// �����ϵͳ���޷������� �Ǿ�ȥfreelistsȡ���ܿ��õ�
			if (!_memstart) {
				// Ų��������freelists
				for (size_t i = get_pos() + 1; i < FREELISTSIZE; i++) {
					if (!_freelists[i]) {
						_memstart = (char*)headpop(i);
						_remain += (i + 1) * algin;
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

#endif // !MR_MALLOC_H


