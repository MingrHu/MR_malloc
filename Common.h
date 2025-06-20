#ifndef COMMON_H
#define COMMON_H
#include<unordered_map> // 后续评价一下性能
#include <mutex>
#include <cassert>

// 引入申请系统内存头文件
// 支持跨平台
#ifdef _WIN32

	#include <Windows.h>

#else  //...Linux

	#include <sys/mman.h>
	#include <unistd.h>	

#endif

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;	// 保证平台通用性
	#define MR_LLMAX 9223372036854775807i64
#elif _WIN32
	typedef size_t PAGE_ID;
	#define MR_LLMAX 2147483647L
#endif // !_WIN64

#define BYTES_BASE_SIZE 128		// 后续返回的实际分配大小是该值的倍数
#define SPAN_MAXNUM 128			// SpanList的长度
#define BYTES_BASE_ALGN 8		// 基础的对齐位数
#define LISTSBASEPOS 15			// 链表的前16个基础下标
#define LISTINTERVAL 8			// 链表后面的下标间隔
#define FREELISTSIZE 104		// 链表的长度
#define CHUNKSIZE 512 * 1024	// 单次申请的最大阈值512KB
#define MAXSIZE 128				// 单次申请的最大块数
#define PAGE_SHIFT 12			// 左移的位数 也就是页大小4KB

// 内存池工具集合
namespace MR_MemPoolToolKits {

	
	// 辅助计算对齐位数的递归模板
	template<size_t Size,size_t Base_Size,size_t Algin,bool Done>
	struct _GetAlginNumHelper {
		static constexpr size_t value = _GetAlginNumHelper< Size, Base_Size << 1, Algin << 1, (Size <= (Base_Size << 1))>::value;
	};

	// 辅助计算对齐位数停止基例
	template<size_t Size, size_t Base_Size,size_t Algin>
	struct _GetAlginNumHelper<Size, Base_Size,Algin,true> {
		static constexpr size_t value = Algin;
	};

	// 辅助计算桶位置的递归模板
	template<size_t Size, size_t Base_Size,size_t cnt,bool Done>
	struct _GetIndexHelper {
		static constexpr size_t value = _GetIndexHelper<Size, Base_Size << 1, cnt + 1, (Size <= (Base_Size << 1))>::value;
	};

	// 辅助计算桶位置停止基例
	template<size_t Size, size_t Base_Size, size_t cnt>
	struct _GetIndexHelper<Size,Base_Size,cnt,true> {
		static constexpr size_t value = cnt;
	};


	// 1.计算返回对齐位数的模板
	template<size_t Size>
	struct _GetAlginNumT {
		static constexpr size_t val = _GetAlginNumHelper<Size, BYTES_BASE_SIZE, BYTES_BASE_ALGN, (Size <= BYTES_BASE_SIZE)>::value;
	};


	// 2.计算返回实际块大小的模板
	template<size_t Size>
	struct _RoundUpT {
		static constexpr size_t Algin = _GetAlginNumT<Size>::val;
		static constexpr size_t val = ((Size - 1) / Algin + 1) * Algin;
	};


	// 3.计算桶位置的模板
	template<size_t Size>
	struct _GetIndexT {
		static constexpr size_t cnt = _GetIndexHelper<Size, BYTES_BASE_SIZE, 0, (Size <= BYTES_BASE_SIZE)>::value;
		static constexpr size_t ex = (_RoundUpT<Size>::val) / _GetAlginNumT<Size>::val - 1;
		static constexpr size_t val = (cnt << 3) + ex;
	};

	// 上述方法的封装
	// 根据请求的字节数 计算对应的实际返回的块大小和对应下标
	template<size_t Size>
	struct SizeClass {

		// 获取当前所在桶
		static size_t _GetIndex() {
			return _GetIndexT<Size>::val;
		}

		// 获取实际返回的块大小
		static inline size_t _RoundUp() {
			return _RoundUpT<Size>::val;
		};

		// 获取对齐位数
		static size_t _GetAlginNum() {
			return _GetAlginNumT<Size>::val;
		}
	};


	// 转为获取常量表达式类型的对象大小
	template<typename T>
	constexpr size_t GetSize() {
		constexpr size_t size = sizeof(T);
		return size;
	}

	// 根据链表下标确定当前分配的空间大小
	static inline size_t GetIndexSize(size_t pos) {

		if (pos <= LISTSBASEPOS) 
			return (pos + 1) * BYTES_BASE_ALGN;
		// 否则根据下标确定的空间 = 基础空间 + pos确定的对齐位数 * 在该对齐位数下的相对位置
		size_t rel_pos = pos - LISTSBASEPOS - (pos - LISTSBASEPOS -1) / LISTINTERVAL * LISTINTERVAL;
		size_t algin = BYTES_BASE_ALGN << ((pos - LISTSBASEPOS - 1) / LISTINTERVAL + 1);
		return algin * rel_pos + (BYTES_BASE_SIZE << ((pos - LISTSBASEPOS - 1) / LISTINTERVAL));
	}

	// 修正ThreadCache向CentralCache申请的内存块个数
	// 依据申请的内存块大小size
	static inline size_t CheckSize(size_t size) {

		assert(size);
		if (CHUNKSIZE / size >= MAXSIZE)
			return MAXSIZE;
		return CHUNKSIZE / size;
	}

	// 修正CentralCache向PageCache申请的页数量
	// 依据申请的内存块大小size
	static inline size_t CheckPageNum(size_t size) {
		size_t m = 1;
		m = max(m, size / (1 << PAGE_SHIFT - 1));
		m = min(m, max(1, size / 64));
		return m;
	}


	//直接去堆上申请按页申请空间
	// pagenum是申请的页个数
	// 左移PAGE_SHIFT是申请8KB * 页数
	static inline void* SystemAlloc(size_t pagenum) {
#ifdef _WIN32

		void* ptr = VirtualAlloc(0, pagenum << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
		// linux下brk mmap等
		void* ptr = mmap(0, kpage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
		if (ptr == nullptr){
			printf("apply for system malloc failed!\n");
			exit(-1);
		}
		return ptr;
	}

	// 跨平台的内存释放函数
	// 记录的 pgid 页的起始地址
	static inline void SystemFree(PAGE_ID pgid) {
		if (pgid == 0) return;
		void* ptr = reinterpret_cast<void*>(pgid << PAGE_SHIFT);

#ifdef _WIN32
		// Windows 下使用 VirtualFree
		if (!VirtualFree(ptr, 0, MEM_RELEASE)) {
			printf("Failed to free memory on Windows!\n");
			exit(-1);
		}
#else
		// Linux/Unix 下使用 munmap
		if (munmap(ptr, pgid << PAGE_SHIFT) == -1) {
			printf("Failed to free memory on Linux/Unix!\n");
			exit(-1);
		}
#endif
	}


	// 计时器 用于统计一段时间 
	// 可用于评估性能
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


	// 管理小块内存的自由链表
	// 支持范围链表的插入和删除
	// 该链表没有内存所有权 析构时无需断开结构
	class _FreeLists {
	public:

		// 防止野指针
		_FreeLists() {
			InitFreeLists();
		}

		// 链表没有内存所有权 析构时无需管理链表结点
		// 内存回收工作交给最上层的PAGE_CACHE
		~_FreeLists()= default;

		// 回收的时候需要初始化
		void InitFreeLists() {
			_freelist_head = nullptr;
			_size = 0;
			_freq = 1;
		}

		// 链表头部插入单个结点
		void headpush(void* back_mem) {
			// 无论是二级还是一级指针本质上就是用
			// 指针存放某个地址 二级指针解引用是内存块的地址
			assert(back_mem);
			Next(back_mem) = _freelist_head;
			_freelist_head = back_mem;
			_size += 1;
		}

		// 获取链表头部单个结点
		void* headpop() {

			void* res = nullptr;
			// 取二级指针的地址 
			assert(_freelist_head);
			void* next = Next(_freelist_head);
			res = _freelist_head;
			_freelist_head = next;
			Next(res) = nullptr;

			_size -= 1;
			return res;
		}

		// 插入一大块结点
		void headRangePush(void* start,void* end,size_t num){

			assert(start);
			assert(end);

			Next(end) = _freelist_head;
			_freelist_head = start;
			_size += num;
		}

		// 传入空的起始点 返回自修正期望num长度的链表
		// start起始结点 numq期望结点数量
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

		// 获取链表头部 通常用于作为分配
		// 范围结点的起始位置
		void* GetListHead() {

			return _freelist_head;
		}

		// 返回一个左值引用 也方便修改
		void*& Next(void* obj) {

			assert(obj && "obj cannot be null");
			return *(void**)obj;
		}

		// 获取链表向下层申请小块内存的频次
		size_t GetFreq() {
			
			return _freq;
		}

		// 增加频次
		void PlusFreq() {
			_freq += 1;
		}

		// 减少频次
		void SubFreq() {
			_freq -= 1;
		}

		// 获取当前链表可用长度
		size_t GetRemainSize() const{
			return _size;
		}

		// 判断链表是否为空
		bool Empty() const{

			return _size == 0;
		}


	private:

		void* _freelist_head;

		size_t _size;		// 定义一个链表数量
		size_t _freq;		// 访问频次

	};


	// 一个Span对象管理一个或多个页
	// Span所管理的页一定是连续的
	class Span {
	public:

		Span* _prev;		// 前驱结点
		Span* _next;		// 后继结点		

		PAGE_ID _pageID;		// 记录起始页号
		PAGE_ID _pageNum;		// 记录分配的页数量			

		size_t _usedcount;		// 已经使用的小块内存数量
		size_t _sum;		// span管理的内存块总数
		bool _isUse;		// 如果这个块已经在CentralCache或者正准备分配给CentralCache 置为true

		_FreeLists _freelist;		// 管理的小块内存链表

		// 默认构造函数
		Span() : _prev(nullptr), _next(nullptr), _pageID(0),
			_pageNum(0), _usedcount(0), _sum(0), _isUse(false), _freelist() {}

		~Span() {
			_prev = nullptr, _next = nullptr;
			_pageID = 0, _pageNum = 0, _usedcount = 0, _sum = 0;
			_isUse = false;
		}
	};

	// 管理不同大小的Span的链表
	// 采用双向链表是为了方便删除和增加结点
	// 执行的增删改查都是单个span
	// 内存池析构时直接释放内存
	class SpanList {
	public:
		SpanList() {
			_spHead = nullptr;
		}


		// 获取span链表头 
		Span* Begin() {

			return _spHead;
		}

		// 获取span链表尾
		Span* End() {

			return nullptr;
		}

		// 删除指定位置span
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

		// 头插
		void headPushSpan(Span* newSpan) {

			assert(newSpan);
			newSpan->_next = _spHead;
			if(_spHead)
				_spHead->_prev = newSpan;
			_spHead = newSpan;
		}

		// 指定位置前插入
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

		// 头出
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

		// 指定位置出
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

		// 可以考虑后期添加运算符重载

		std::mutex _mtx;		// 桶锁

	private:
		
		Span* _spHead;

	};

	// 定长内存池
	template <class T>
	class MemoryPool {
	public:
		MemoryPool() {
			_remain = 0;
			_memstart = nullptr;
		}

		T* Allocate() {
			// 超出单次的最大申请内存阈值
			constexpr size_t _memSize = GetSize<T>();
			if (_memSize > MAXSIZE)
				return (T*)::operator new(_memSize);

			// 获取实际对齐位数以及预期分配的空间大小
			size_t algin = SizeClass<_memSize>::_GetAlginNum();
			size_t chunk_size = SizeClass<_memSize>::_RoundUp();
			size_t pos = get_pos();

			if (_freelists[pos].Empty()) {
				// 从chunk_alloc里面拿一大块内存 同时补充_free_lists
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

		// 记录分配的大块内存起始地址 用于释放
		std::vector<char*> _startrecord;

		size_t get_pos() const {
			constexpr size_t _memSize = GetSize<T>();
			return SizeClass<_memSize>::_GetIndex();
		}

		// 重要的块分配函数
		// n是请求的单个块大小 
		char* chunk_alloc(size_t n, size_t& num, size_t algin) {
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
				_memstart = (char*)::operator new(chunk);
				// 如果连系统都无法分配了 那就去freelists取可能可用的
				if (!_memstart) {
					// 挪动后续的freelists
					for (size_t i = pos + 1; i < FREELISTSIZE; i++) {
						if (!_freelists[i].Empty()) {
							_memstart = (char*)_freelists[i].headpop();
							_remain += GetIndexSize(i);
							// 进入alloc再次去修正num
							return chunk_alloc(n, num, algin);
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
				return chunk_alloc(n, num, algin);
			}
		}// chunk_alloc 
	};


	// 定长内存池维护的span对象
	// 仅给pagecache使用
	class SpanPool {
	public:
		// 获取新的span指针对象
		Span* _spAllocate() {
			// 未初始化内存
			Span* newSpanMemory = _spPool.Allocate();
			Span* newSpan = new (newSpanMemory) Span(); // 调用默认构造函数

			return newSpan;
		}

		// 回收span指针对象
		void _spDellocate(Span* back_span) {
			_spPool.DeAllocate(back_span);
		}

	private:
		// 对象池
		MemoryPool<Span> _spPool;
	};

}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
