#ifndef MR_MALLOC_H
#include "ThreadCache.h"
#include "Common.h"
#define DYNAMIC 1
#define STATIC 1
#ifdef DYNAMIC
	// 运行时确定
	class MR_malloc {
	public:

		static void* Allocate(size_t memSize) {
			size_t pos = 
			return tls_threadcache.Allocate(pos, memSize);
		}

		static void Dellocate(void* obj) {
			assert(obj);
			size_t pos =
			tls_threadcache.DeAllocate(obj, pos);
		}

	private:	

		static inline size_t _CalRoundUp(size_t Size) {
			size_t algin = _Cal_Algin(Size);
			return ((Size + algin - 1) & ~(algin - 1));
		}

		static inline size_t _Cal_Algin(size_t Size) {

			if (Size <= 128)
				return 8;
			else if (Size <= 256)
				return 16;
			else if (Size <= 512)
				return 32;
			else if (Size <= 1024)
				return 64;
			else if (Size <= 2 * 1024)
				return 128;
			else if (Size <= 4 * 1024)
				return 256;
			else if (Size <= 8 * 1024)
				return 512;
			else if (Size <= 16 * 1024)
				return 1024;
			else if (Size <= 32 * 1024)
				return 2 * 1024;
			else if (Size <= 64 * 1024)
				return 4 * 1024;
			else if (Size <= 128 * 1024)
				return 8 * 1024;
			else return 16 * 1024;
		}

		int vec[]
	};



#elif STATIC
	// 编译时确定
	template<typename T>
	class MR_malloc {
	public:
		static T* Allocate() {
			constexpr size_t _memSize = GetSize<T>();
			size_t pos = SizeClass<_memSize>::_GetIndex();
			return (T*)tls_threadcache.Allocate(pos, _memSize);
		}

		static void Dellocate(T* obj) {
			assert(obj);
			constexpr size_t _memSize = GetSize<T>();
			size_t pos = SizeClass<_memSize>::_GetIndex();
			tls_threadcache.DeAllocate(obj, pos);
		}
	};
#endif // DYNAMIC or STATIC

#endif // !MR_MALLOC_H
