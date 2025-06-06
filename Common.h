#ifndef COMMON_H
#define COMMON_H
#define BYTES_BASE_SIZE 128
#define BYTES_BASE_ALGN 8
#define MAX_CYCLENUM 11
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

	// 辅助计算桶停止基例
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
		constexpr size_t size= sizeof(T);
		return size;
	}


}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
