#ifndef MR_MALLOC
#define MR_MACLLOC
#include<iostream>
#include<type_traits>
#include<vector>
#define FREELISTSIZE 16 // ����������б����
#define CHUNKNUM 128
#define CHUNKSIZE 8  // ��С�Ŀ��С
#define MAXSIZE 128 // ����������ڴ��С��ֵ
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
*  ��ģ�麯����
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
				// ��chunk_alloc������һ����ڴ� ͬʱ����_free_lists
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
		// ��¼����Ĵ���ڴ���ʼ��ַ �����ͷ�
		std::vector<char*> _startrecord;

		void headpush(T* obj) {

			obj->~T();
			int pos = get_pos(sizeof(T));
			// �����Ƕ�������һ��ָ�뱾���Ͼ�����
			// ָ����ĳ����ַ ����ָ����������ڴ��ĵ�ַ
			*(void**)obj = _freelists[pos];
			_freelists[pos] = obj;
		}

		size_t get_pos(size_t n) const{
			return (n - 1) / CHUNKSIZE;
		}

		void* headpop(size_t pos) {
			// ȡ����ָ���ŵĵ�ַ
			void* next = *(void**)_freelists[pos];
			// ȡ����ָ��ĵ�ַ Ҳ����֮ǰ���տ����ڴ�obj��ַ
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
				// ե��ʣ���ֵ
				while (_remain > 0) {
					int m = _remain / CHUNKSIZE - 1;
					if (m < 0)break;
					_remain -= CHUNKSIZE * (m + 1);
					*(void**)_memstart = _freelists[m];
					_freelists[m] = _memstart;
				}
				_remain = 0;
				_memstart =(char*)malloc(chunk);
				// �����ϵͳ���޷������� �Ǿ�ȥfreelistȡ���ܿ��õ�
				if (!_memstart) {
					// Ų��������freelists
					for (size_t i = get_pos(n) + 1; i < FREELISTSIZE; i++) {
						if (!_freelists[i]) {
							_memstart = (char*)headpop(i);
							_remain += (i + 1) * CHUNKSIZE;
							// ����alloc�ٴ�ȥ����num
							return chunk_alloc(n, num);
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
				return chunk_alloc(n, num);
			}
		}
	};
};

#endif // !MR_MALLOC


