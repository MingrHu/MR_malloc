#ifndef MR_MALLOC_H
#include "ThreadCache.h"
#include "Common.h"
#define DYNAMIC 1
#define STATIC 0
#define PRIVATE 0

#if DYNAMIC
	// 运行时确定
	class MR_malloc {
	public:

		static void* Allocate(size_t memSize) {
			if (memSize == 0)return nullptr;
			std::pair<int, int> p = _CalRoundUp(memSize);
			if (tls_threadcache == nullptr)
				tls_threadcache = std::make_unique<ThreadCache>();
			return tls_threadcache->Allocate(p.second, p.first);
		}

		static void Dellocate(void* obj,size_t memSize) {
			if (!obj)return;
			std::pair<int, int> p = _CalRoundUp(memSize);
			tls_threadcache->DeAllocate(obj, p.second);
		}

#if  PRIVATE
	private:	
#endif
		// 第一个返回对齐大小 第二个返回位置
		static inline std::pair<size_t, size_t> _CalRoundUp(size_t Size) {
			std::pair<size_t, size_t> p = _CalFun(Size);
			size_t mem_size = ((Size + p.first - 1) & ~(p.first - 1));
			size_t pos = (mem_size - _predixSize[p.second]) / p.first + _predixSum[p.second];
			return {mem_size,pos};
		}

		// 返回两个参数 第一个是对齐规则 第二个是前缀数组下标
		static inline std::pair<size_t,size_t> _CalFun(size_t Size) {
			
			if (Size <= 128)
				return { 8,0 };
			else if (Size <= 256)
				return { 16,1 };
			else if (Size <= 512)
				return { 32,2 };
			else if (Size <= 1024)
				return { 64,3 };
			else if (Size <= 2 * 1024)
				return { 128,4 };
			else if (Size <= 4 * 1024)
				return { 256,5 };
			else if (Size <= 8 * 1024)
				return { 512,6 };
			else if (Size <= 16 * 1024)
				return { 1024,7 };
			else if (Size <= 32 * 1024)
				return { 2 * 1024,8 };
			else if (Size <= 64 * 1024)
				return { 4 * 1024,9 };
			else if (Size <= 128 * 1024)
				return { 8 * 1024,10 };
			else return { 16 * 1024,11 };
		}

		static constexpr int _predixSum[12] = {-1,15,23,31,39,47,55,63,71,79,87,95};
		static constexpr int _predixSize[12] = { 0,128,256,512,1024,2 * 1024,4 * 1024,8 * 1024,16 * 1024,32 * 1024,64 * 1024,128 * 1024 };
		
	};
#elif STATIC
	// 编译时确定
	template<typename T>
	class MR_malloc {
	public:
		static T* Allocate() {
			// 超出管理的范围如何处理？
			constexpr size_t _memSize = GetSize<T>();
			assert(_memSize);
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
