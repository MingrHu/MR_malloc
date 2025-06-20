#ifndef COMMON_H
#define COMMON_H
#include<unordered_map> // ��������һ������
#include <mutex>
#include <cassert>

// ��������ϵͳ�ڴ�ͷ�ļ�
// ֧�ֿ�ƽ̨
#ifdef _WIN32

	#include <Windows.h>

#else  //...Linux

	#include <sys/mman.h>
	#include <unistd.h>	

#endif

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;	// ��֤ƽ̨ͨ����
	#define MR_LLMAX 9223372036854775807i64
#elif _WIN32
	typedef size_t PAGE_ID;
	#define MR_LLMAX 2147483647L
#endif // !_WIN64

#define BYTES_BASE_SIZE 128		// �������ص�ʵ�ʷ����С�Ǹ�ֵ�ı���
#define SPAN_MAXNUM 128			// SpanList�ĳ���
#define BYTES_BASE_ALGN 8		// �����Ķ���λ��
#define LISTSBASEPOS 15			// �����ǰ16�������±�
#define LISTINTERVAL 8			// ���������±���
#define FREELISTSIZE 104		// ����ĳ���
#define CHUNKSIZE 512 * 1024	// ��������������ֵ512KB
#define MAXSIZE 128				// ���������������
#define PAGE_SHIFT 12			// ���Ƶ�λ�� Ҳ����ҳ��С4KB

// �ڴ�ع��߼���
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

	// ����ThreadCache��CentralCache������ڴ�����
	// ����������ڴ���Сsize
	static inline size_t CheckSize(size_t size) {

		assert(size);
		if (CHUNKSIZE / size >= MAXSIZE)
			return MAXSIZE;
		return CHUNKSIZE / size;
	}

	// ����CentralCache��PageCache�����ҳ����
	// ����������ڴ���Сsize
	static inline size_t CheckPageNum(size_t size) {
		size_t m = 1;
		m = max(m, size / (1 << PAGE_SHIFT - 1));
		m = min(m, max(1, size / 64));
		return m;
	}


	//ֱ��ȥ�������밴ҳ����ռ�
	// pagenum�������ҳ����
	// ����PAGE_SHIFT������8KB * ҳ��
	static inline void* SystemAlloc(size_t pagenum) {
#ifdef _WIN32

		void* ptr = VirtualAlloc(0, pagenum << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
		// linux��brk mmap��
		void* ptr = mmap(0, kpage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
		if (ptr == nullptr){
			printf("apply for system malloc failed!\n");
			exit(-1);
		}
		return ptr;
	}

	// ��ƽ̨���ڴ��ͷź���
	// ��¼�� pgid ҳ����ʼ��ַ
	static inline void SystemFree(PAGE_ID pgid) {
		if (pgid == 0) return;
		void* ptr = reinterpret_cast<void*>(pgid << PAGE_SHIFT);

#ifdef _WIN32
		// Windows ��ʹ�� VirtualFree
		if (!VirtualFree(ptr, 0, MEM_RELEASE)) {
			printf("Failed to free memory on Windows!\n");
			exit(-1);
		}
#else
		// Linux/Unix ��ʹ�� munmap
		if (munmap(ptr, pgid << PAGE_SHIFT) == -1) {
			printf("Failed to free memory on Linux/Unix!\n");
			exit(-1);
		}
#endif
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


	// ����С���ڴ����������
	// ֧�ַ�Χ����Ĳ����ɾ��
	// ������û���ڴ�����Ȩ ����ʱ����Ͽ��ṹ
	class _FreeLists {
	public:

		// ��ֹҰָ��
		_FreeLists() {
			InitFreeLists();
		}

		// ����û���ڴ�����Ȩ ����ʱ�������������
		// �ڴ���չ����������ϲ��PAGE_CACHE
		~_FreeLists()= default;

		// ���յ�ʱ����Ҫ��ʼ��
		void InitFreeLists() {
			_freelist_head = nullptr;
			_size = 0;
			_freq = 1;
		}

		// ����ͷ�����뵥�����
		void headpush(void* back_mem) {
			// �����Ƕ�������һ��ָ�뱾���Ͼ�����
			// ָ����ĳ����ַ ����ָ����������ڴ��ĵ�ַ
			assert(back_mem);
			Next(back_mem) = _freelist_head;
			_freelist_head = back_mem;
			_size += 1;
		}

		// ��ȡ����ͷ���������
		void* headpop() {

			void* res = nullptr;
			// ȡ����ָ��ĵ�ַ 
			assert(_freelist_head);
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

		// ����յ���ʼ�� ��������������num���ȵ�����
		// start��ʼ��� numq�����������
		void headRangePop(void*& start,void*&end,size_t& num) {
			
			assert(_freelist_head && "Freelist head nullptr!");
			start = _freelist_head;
			end = start;
			num = min(_size, num);
			for (size_t i = 0; i < num - 1; i++) 
				end = Next(end);
			_freelist_head = Next(end);
			_size -= num;
			Next(end) = nullptr;
		}

		// ��ȡ����ͷ�� ͨ��������Ϊ����
		// ��Χ������ʼλ��
		void* GetListHead() {

			return _freelist_head;
		}

		// ����һ����ֵ���� Ҳ�����޸�
		void*& Next(void* obj) {

			assert(obj && "obj cannot be null");
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
	// Span�������ҳһ����������
	class Span {
	public:

		Span* _prev;		// ǰ�����
		Span* _next;		// ��̽��		

		PAGE_ID _pageID;		// ��¼��ʼҳ��
		PAGE_ID _pageNum;		// ��¼�����ҳ����			

		size_t _usedcount;		// �Ѿ�ʹ�õ�С���ڴ�����
		size_t _sum;		// span������ڴ������
		bool _isUse;		// ���������Ѿ���CentralCache������׼�������CentralCache ��Ϊtrue

		_FreeLists _freelist;		// �����С���ڴ�����

		// Ĭ�Ϲ��캯��
		Span() : _prev(nullptr), _next(nullptr), _pageID(0),
			_pageNum(0), _usedcount(0), _sum(0), _isUse(false), _freelist() {}

		~Span() {
			_prev = nullptr, _next = nullptr;
			_pageID = 0, _pageNum = 0, _usedcount = 0, _sum = 0;
			_isUse = false;
		}
	};

	// ����ͬ��С��Span������
	// ����˫��������Ϊ�˷���ɾ�������ӽ��
	// ִ�е���ɾ�Ĳ鶼�ǵ���span
	// �ڴ������ʱֱ���ͷ��ڴ�
	class SpanList {
	public:
		SpanList() {
			_spHead = nullptr;
		}


		// ��ȡspan����ͷ 
		Span* Begin() {

			return _spHead;
		}

		// ��ȡspan����β
		Span* End() {

			return nullptr;
		}

		// ɾ��ָ��λ��span
		void Erase(Span* pos) {

			assert(pos);

			Span* prev = pos->_prev;
			Span* next = pos->_next;

			// 
			if (prev) prev->_next = next;
			else _spHead = next;
			if (next) next->_prev = prev;

			pos->_prev = nullptr;
			pos->_next = nullptr;
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
			if (pos->_prev) {
				newSpan->_prev = pos->_prev;
				pos->_prev->_next = newSpan;
			}
			pos->_prev = newSpan;
		}

		// ͷ��
		Span* headPopSpan() {
			
			if (_spHead) {
				Span* span = _spHead;
				_spHead = _spHead->_next;
				if (_spHead)_spHead->_prev = nullptr;
				span->_next = nullptr;
				return span;
			}
			return nullptr;
		}

		// ָ��λ�ó�
		Span* popSpan(Span* pos) {
			
			assert(pos);
			if (pos == _spHead)
				return headPopSpan();
			Span* prev = pos->_prev;
			Span * next = pos->_next;
			if (prev)prev->_next = next;
			if (next) next->_prev = prev;
			pos->_prev = nullptr, pos->_next = nullptr;
			return pos;
		}

		// ���Կ��Ǻ���������������

		std::mutex _mtx;		// Ͱ��

	private:
		
		Span* _spHead;

	};

	// �����ڴ��
	template <class T>
	class MemoryPool {
	public:
		MemoryPool() {
			_remain = 0;
			_memstart = nullptr;
		}

		T* Allocate() {
			// �������ε���������ڴ���ֵ
			constexpr size_t _memSize = GetSize<T>();
			if (_memSize > MAXSIZE)
				return (T*)::operator new(_memSize);

			// ��ȡʵ�ʶ���λ���Լ�Ԥ�ڷ���Ŀռ��С
			size_t algin = SizeClass<_memSize>::_GetAlginNum();
			size_t chunk_size = SizeClass<_memSize>::_RoundUp();
			size_t pos = get_pos();

			if (_freelists[pos].Empty()) {
				// ��chunk_alloc������һ����ڴ� ͬʱ����_free_lists
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
			assert(obj);
			obj->~T();
			size_t pos = get_pos();
			_freelists[pos].headpush(obj);
		}

		~MemoryPool() {
			for (auto mem : _startrecord)
				::operator delete(mem);
		}

	private:

		_FreeLists _freelists[FREELISTSIZE];
		char* _memstart;
		size_t _remain;
		static constexpr size_t _memSize = GetSize<T>();

		// ��¼����Ĵ���ڴ���ʼ��ַ �����ͷ�
		std::vector<char*> _startrecord;

		size_t get_pos() const {
			constexpr size_t _memSize = GetSize<T>();
			return SizeClass<_memSize>::_GetIndex();
		}

		// ��Ҫ�Ŀ���亯��
		// n������ĵ������С 
		char* chunk_alloc(size_t n, size_t& num, size_t algin) {
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
				_memstart = (char*)::operator new(chunk);
				// �����ϵͳ���޷������� �Ǿ�ȥfreelistsȡ���ܿ��õ�
				if (!_memstart) {
					// Ų��������freelists
					for (size_t i = pos + 1; i < FREELISTSIZE; i++) {
						if (!_freelists[i].Empty()) {
							_memstart = (char*)_freelists[i].headpop();
							_remain += GetIndexSize(i);
							// ����alloc�ٴ�ȥ����num
							return chunk_alloc(n, num, algin);
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
				return chunk_alloc(n, num, algin);
			}
		}// chunk_alloc 
	};


	// �����ڴ��ά����span����
	// ����pagecacheʹ��
	class SpanPool {
	public:
		// ��ȡ�µ�spanָ�����
		Span* _spAllocate() {
			// δ��ʼ���ڴ�
			Span* newSpanMemory = _spPool.Allocate();
			Span* newSpan = new (newSpanMemory) Span(); // ����Ĭ�Ϲ��캯��

			return newSpan;
		}

		// ����spanָ�����
		void _spDellocate(Span* back_span) {
			_spPool.DeAllocate(back_span);
		}

	private:
		// �����
		MemoryPool<Span> _spPool;
	};

}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
