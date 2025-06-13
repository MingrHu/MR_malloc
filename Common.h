#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;	// 保证平台通用性
#define MR_LLMAX 9223372036854775807i64
#elif _WIN32
	typedef size_t PAGE_ID;
#define MR_LLMAX 2147483647L
#endif // !_WIN64

#include <mutex>
#include <cassert>

#define BYTES_BASE_SIZE 128		// 后续返回的实际分配大小是该值的倍数
#define BYTES_BASE_ALGN 8		// 基础的对齐位数
#define LISTSBASEPOS 15			// 链表的前16个基础下标
#define LISTINTERVAL 8			// 链表后面的下标间隔
#define FREELISTSIZE 104		// 自由链表的列表个数
#define CHUNKSIZE 1024 * 1024	// 单次申请的最大阈值1MB
#define MAXSIZE 512				// 单次申请的最大块数
#define PAGE_SHIFT 12			// 左移的位数 也就是页大小4KB

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

	static inline size_t CheckSize(size_t size) {

		if (CHUNKSIZE / size > MAXSIZE)
			return MAXSIZE;
		return CHUNKSIZE / size;
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

	
	
	class _FreeLists {
	public:

		// 防止野指针
		_FreeLists() {
			_freelist_head = nullptr;
			_size = 0;
			_freq = 1;
		}

		// 防止内存泄露 链表所保存的内存地址要清零 
		// 内存回收工作交给最上层的PAGE_CACHE
		~_FreeLists() {
			while (_size) {
				_size -= 1;
				void* next = Next(_freelist_head);
				Next(_freelist_head) = nullptr;
				_freelist_head = next;
			}
		}

		// 插入单个结点
		void headpush(void* back_mem) {
			// 无论是二级还是一级指针本质上就是用
			// 指针存放某个地址 二级指针解引用是内存块的地址
			assert(back_mem);
			Next(back_mem) = _freelist_head;
			_freelist_head = back_mem;
			_size += 1;
		}

		// 获取单个结点
		void* headpop() {

			void* res = nullptr;
			// 取二级指针的地址 也就是之前回收可用内存obj地址
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

		// 给一个起始点 返回自修正期望num长度的链表
		// start起始结点 end结束结点 num结点数量
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

		// 获取链表头部 通常用于作为分配
		// 范围结点的起始位置
		void* GetListHead() {

			return _freelist_head;
		}

		// 返回一个左值引用 也方便修改
		void*& Next(void* obj) {

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
	struct Span {

		Span* _prev = nullptr;
		Span* _next = nullptr;

		PAGE_ID _pageID = 0;		// 记录起始页号
		PAGE_ID _pageNum = 0;		// 记录分配的页数量

		size_t _usedcount = 0;		// 已经使用的小块内存数量
		bool _isUse = false;		// 如果这个块已经在CentralCache或者正准备分配给CentralCache 置为true

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

		// 删除指定位置span
		void Erase(Span* pos) {
			
			assert(pos);
			if (pos->_prev)
				pos->_prev->_next = pos->_next;
			else _spHead = pos->_next;
			delete pos;
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
			if (pos->_prev) 
				newSpan->_prev = pos->_prev;
			pos->_prev = newSpan;
		}

		// 头出
		Span* headPopSpan() {
			
			if (_spHead) {
				_spHead = _spHead->_next;
				return _spHead;
			}
			return nullptr;
		}

		// 指定位置出
		Span* popSpan(Span* pos) {
			
			assert(pos);
			if (pos->_prev)
				pos->_prev->_next = pos->_next;
			else _spHead = pos->_next;
			return pos;
		}

		// 可以考虑后期添加运算符重载

		std::mutex _mtx;		// 桶锁

	private:
		
		Span* _spHead;

	};

}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
