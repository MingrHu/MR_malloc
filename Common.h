#ifndef COMMON_H
#define COMMON_H
#define BYTES_BASE_SIZE 128
#define BYTES_BASE_ALGN 8
#define MAX_CYCLENUM 11
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

	// ��������Ͱֹͣ����
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
		constexpr size_t size= sizeof(T);
		return size;
	}


}; // namespace MR_MemPoolToolKits
#endif // ! COMMON_H
