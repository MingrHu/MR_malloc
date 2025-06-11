#ifndef PAGECACHE_H
#define PAGECACHE_H
#include"Common.h"
using namespace MR_MemPoolToolKits;

class PageCache {
public:

	static PageCache* getInstance() {
		return &_PageInstance;
	}

	// num是请求的页个数 根据页个数可以直接找对应的span
	Span* FetchNewSpan(size_t num);

private:

	PageCache() = default;

	PageCache(const PageCache& copy)noexcept = delete;
	PageCache& operator=(const PageCache& copy)noexcept = delete;

	PageCache(const PageCache&& copy)noexcept = delete;
	PageCache& operator=(const PageCache&& copy)noexcept = delete;

	static PageCache _PageInstance;

	std::mutex _pgmtx;
};



#endif // !PAGECACHE_H

