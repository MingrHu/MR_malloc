#ifndef COMMON_H
#define COMMON_H
#define BYTES_BASE_SIZE 128 // 后续返回的实际分配大小是该值的倍数
#define BYTES_BASE_ALGN 8   // 基础的对齐位数
#define LISTSBASEPOS 15		// 链表的前16个基础下标
#define LISTINTERVAL 8		// 链表后面的下标间隔
#define FREELISTSIZE 104	// 自由链表的列表个数
#define CHUNKNUM 512		// 
#define MAXSIZE 256 * 1024	// 单次申请的内存大小阈值
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
	size_t GetIndexSize(size_t pos) {
		if (pos <= LISTSBASEPOS) 
			return (pos + 1) * BYTES_BASE_ALGN;
		// 否则根据下标确定的空间 = 基础空间 + pos确定的对齐位数 * 在该对齐位数下的相对位置
		size_t rel_pos = pos - LISTSBASEPOS - (pos - LISTSBASEPOS -1) / LISTINTERVAL * LISTINTERVAL;
		size_t algin = BYTES_BASE_ALGN << ((pos - LISTSBASEPOS - 1) / LISTINTERVAL + 1);
		return algin * rel_pos + BYTES_BASE_SIZE << ((pos - LISTSBASEPOS - 1) / LISTINTERVAL);
	}
	
	class _FreeLists {
	public:

		// 防止野指针
		_FreeLists() {
			_freelist_head = nullptr;
		}

		// 防止内存泄露 链表所保存的内存地址要清零 
		// 内存回收工作交给最上层的PAGE_CACHE
		~_FreeLists() {
			_freelist_head = nullptr;
		}

		void headpush(void* back_mem) {
			// 无论是二级还是一级指针本质上就是用
			// 指针存放某个地址 二级指针解引用是内存块的地址
			*(void**)back_mem = _freelist_head;
			_freelist_head = back_mem;
		}

		void* headpop() {
			void* res = nullptr;
			// 取二级指针的地址 也就是之前回收可用内存obj地址
			void* next = *(void**)_freelist_head;
			res = _freelist_head;
			_freelist_head = next;
			return res;
		}

		bool empty() {
			return _freelist_head == nullptr;
		}

	private:
		void* _freelist_head;
	};

}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
