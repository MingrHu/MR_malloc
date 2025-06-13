#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;	// ��֤ƽ̨ͨ����
#define MR_LLMAX 9223372036854775807i64
#elif _WIN32
	typedef size_t PAGE_ID;
#define MR_LLMAX 2147483647L
#endif // !_WIN64

#include <mutex>
#include <cassert>

#define BYTES_BASE_SIZE 128		// �������ص�ʵ�ʷ����С�Ǹ�ֵ�ı���
#define BYTES_BASE_ALGN 8		// �����Ķ���λ��
#define LISTSBASEPOS 15			// �����ǰ16�������±�
#define LISTINTERVAL 8			// ���������±���
#define FREELISTSIZE 104		// ����������б����
#define CHUNKSIZE 1024 * 1024	// ��������������ֵ1MB
#define MAXSIZE 512				// ���������������
#define PAGE_SHIFT 12			// ���Ƶ�λ�� Ҳ����ҳ��С4KB

namespace MR_MemPoolToolKits {

	// �����������λ���ĵݹ�ģ��
	template<size_t Size,size_t Base_Size,size_t Algin,bool Done>
	struct _GetAlginNumHelper {
		static constexpr size_t value = _GetAlginNumHelper< Size, Base_Size << 1, Algin << 1, (Size <= (Base_Size << 1))>::value;
	};

	// �����������λ��ֹͣ����
	template<size_t Size, size_t Base_Size,size_t Algin>
	struct _GetAlginNumHelper<Size, Base_Size,Algin,true> {
		static constexpr size_t value = Algin;
	};

	// ��������Ͱλ�õĵݹ�ģ��
	template<size_t Size, size_t Base_Size,size_t cnt,bool Done>
	struct _GetIndexHelper {
		static constexpr size_t value = _GetIndexHelper<Size, Base_Size << 1, cnt + 1, (Size <= (Base_Size << 1))>::value;
	};

	// ��������Ͱλ��ֹͣ����
	template<size_t Size, size_t Base_Size, size_t cnt>
	struct _GetIndexHelper<Size,Base_Size,cnt,true> {
		static constexpr size_t value = cnt;
	};


	// 1.���㷵�ض���λ����ģ��
	template<size_t Size>
	struct _GetAlginNumT {
		static constexpr size_t val = _GetAlginNumHelper<Size, BYTES_BASE_SIZE, BYTES_BASE_ALGN, (Size <= BYTES_BASE_SIZE)>::value;
	};


	// 2.���㷵��ʵ�ʿ��С��ģ��
	template<size_t Size>
	struct _RoundUpT {
		static constexpr size_t Algin = _GetAlginNumT<Size>::val;
		static constexpr size_t val = ((Size - 1) / Algin + 1) * Algin;
	};


	// 3.����Ͱλ�õ�ģ��
	template<size_t Size>
	struct _GetIndexT {
		static constexpr size_t cnt = _GetIndexHelper<Size, BYTES_BASE_SIZE, 0, (Size <= BYTES_BASE_SIZE)>::value;
		static constexpr size_t ex = (_RoundUpT<Size>::val) / _GetAlginNumT<Size>::val - 1;
		static constexpr size_t val = (cnt << 3) + ex;
	};

	// ���������ķ�װ
	// ����������ֽ��� �����Ӧ��ʵ�ʷ��صĿ��С�Ͷ�Ӧ�±�
	template<size_t Size>
	struct SizeClass {

		// ��ȡ��ǰ����Ͱ
		static size_t _GetIndex() {
			return _GetIndexT<Size>::val;
		}

		// ��ȡʵ�ʷ��صĿ��С
		static inline size_t _RoundUp() {
			return _RoundUpT<Size>::val;
		};

		// ��ȡ����λ��
		static size_t _GetAlginNum() {
			return _GetAlginNumT<Size>::val;
		}
	};

	// תΪ��ȡ�������ʽ���͵Ķ����С
	template<typename T>
	constexpr size_t GetSize() {
		constexpr size_t size = sizeof(T);
		return size;
	}

	// ���������±�ȷ����ǰ����Ŀռ��С
	static inline size_t GetIndexSize(size_t pos) {

		if (pos <= LISTSBASEPOS) 
			return (pos + 1) * BYTES_BASE_ALGN;
		// ��������±�ȷ���Ŀռ� = �����ռ� + posȷ���Ķ���λ�� * �ڸö���λ���µ����λ��
		size_t rel_pos = pos - LISTSBASEPOS - (pos - LISTSBASEPOS -1) / LISTINTERVAL * LISTINTERVAL;
		size_t algin = BYTES_BASE_ALGN << ((pos - LISTSBASEPOS - 1) / LISTINTERVAL + 1);
		return algin * rel_pos + (BYTES_BASE_SIZE << ((pos - LISTSBASEPOS - 1) / LISTINTERVAL));
	}

	static inline size_t CheckSize(size_t size) {

		if (CHUNKSIZE / size > MAXSIZE)
			return MAXSIZE;
		return CHUNKSIZE / size;
	}

	// ��ʱ�� ����ͳ��һ��ʱ�� 
	// ��������������
	class Timer {
	public:
		Timer() = default;

		void init() {
			_start = std::chrono::high_resolution_clock::now();
		}

		long long elapsed() const {
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - _start);
			return duration.count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _start;
	};

	
	
	class _FreeLists {
	public:

		// ��ֹҰָ��
		_FreeLists() {
			_freelist_head = nullptr;
			_size = 0;
			_freq = 1;
		}

		// ��ֹ�ڴ�й¶ ������������ڴ��ַҪ���� 
		// �ڴ���չ����������ϲ��PAGE_CACHE
		~_FreeLists() {
			while (_size) {
				_size -= 1;
				void* next = Next(_freelist_head);
				Next(_freelist_head) = nullptr;
				_freelist_head = next;
			}
		}

		// ���뵥�����
		void headpush(void* back_mem) {
			// �����Ƕ�������һ��ָ�뱾���Ͼ�����
			// ָ����ĳ����ַ ����ָ����������ڴ��ĵ�ַ
			assert(back_mem);
			Next(back_mem) = _freelist_head;
			_freelist_head = back_mem;
			_size += 1;
		}

		// ��ȡ�������
		void* headpop() {

			void* res = nullptr;
			// ȡ����ָ��ĵ�ַ Ҳ����֮ǰ���տ����ڴ�obj��ַ
			void* next = Next(_freelist_head);
			res = _freelist_head;
			_freelist_head = next;
			Next(res) = nullptr;

			_size -= 1;
			return res;
		}

		// ����һ�����
		void headRangePush(void* start,void* end,size_t num){

			assert(start);
			assert(end);

			Next(end) = _freelist_head;
			_freelist_head = start;
			_size += num;
		}

		// ��һ����ʼ�� ��������������num���ȵ�����
		// start��ʼ��� end������� num�������
		void headRangePop(void*& start,void*& end,size_t &num) {


			assert(start);
			start = _freelist_head;
			end = start;

			size_t actnum = 1;
			while (Next(end) && num - 1) {
				end = Next(end);
				actnum += 1;
			}
			num = actnum;
			_freelist_head = Next(end);
			_size -= num;
		}

		// ��ȡ����ͷ�� ͨ��������Ϊ����
		// ��Χ������ʼλ��
		void* GetListHead() {

			return _freelist_head;
		}

		// ����һ����ֵ���� Ҳ�����޸�
		void*& Next(void* obj) {

			return *(void**)obj;
		}

		// ��ȡ�������²�����С���ڴ��Ƶ��
		size_t GetFreq() {

			return _freq;
		}

		// ����Ƶ��
		void PlusFreq() {
			_freq += 1;
		}

		// ����Ƶ��
		void SubFreq() {
			_freq -= 1;
		}

		// ��ȡ��ǰ������ó���
		size_t GetRemainSize() const{
			return _size;
		}

		// �ж������Ƿ�Ϊ��
		bool Empty() const{

			return _size == 0;
		}


	private:

		void* _freelist_head;

		size_t _size;		// ����һ����������
		size_t _freq;		// ����Ƶ��

	};

	// һ��Span�������һ������ҳ
	struct Span {

		Span* _prev = nullptr;
		Span* _next = nullptr;

		PAGE_ID _pageID = 0;		// ��¼��ʼҳ��
		PAGE_ID _pageNum = 0;		// ��¼�����ҳ����

		size_t _usedcount = 0;		// �Ѿ�ʹ�õ�С���ڴ�����
		bool _isUse = false;		// ���������Ѿ���CentralCache������׼�������CentralCache ��Ϊtrue

		_FreeLists _freelist;
	};

	
	class SpanList {
	public:
		SpanList() {
			_spHead = nullptr;
		}

		~SpanList() {
			while (_spHead) {
				Erase(_spHead);
			}
		}

		Span* Begin() {

			return _spHead;
		}

		Span* End() {

			return nullptr;
		}

		// ɾ��ָ��λ��span
		void Erase(Span* pos) {
			
			assert(pos);
			if (pos->_prev)
				pos->_prev->_next = pos->_next;
			else _spHead = pos->_next;
			delete pos;
		}

		// ͷ��
		void headPushSpan(Span* newSpan) {

			assert(newSpan);
			newSpan->_next = _spHead;
			if(_spHead)
				_spHead->_prev = newSpan;
			_spHead = newSpan;
		}

		// ָ��λ��ǰ����
		void insertSpan(Span* pos,Span* newSpan) {
			
			assert(pos);
			assert(newSpan);
			newSpan->_next = pos;
			if (pos->_prev) 
				newSpan->_prev = pos->_prev;
			pos->_prev = newSpan;
		}

		// ͷ��
		Span* headPopSpan() {
			
			if (_spHead) {
				_spHead = _spHead->_next;
				return _spHead;
			}
			return nullptr;
		}

		// ָ��λ�ó�
		Span* popSpan(Span* pos) {
			
			assert(pos);
			if (pos->_prev)
				pos->_prev->_next = pos->_next;
			else _spHead = pos->_next;
			return pos;
		}

		// ���Կ��Ǻ���������������

		std::mutex _mtx;		// Ͱ��

	private:
		
		Span* _spHead;

	};

}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
