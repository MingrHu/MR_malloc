#ifndef PAGECACHE_H
#define PAGECACHE_H
#include"Common.h"
using namespace MR_MemPoolToolKits;
// 页缓存
// 单例设计
class PageCache {
public:

	static PageCache* getInstance() {
		return &_PageInstance;
	}

	// num是请求的页个数 根据页个数可以直接找对应的span
	Span* FetchNewSpan(size_t num);

	// 获取地址页号和span*的对应关系
	Span* GetHashObjwithSpan(void* obj);

	// 返还来自ThreadCache的span至pagecache
	// 优先把零碎的组织起来成大的span
	void ReleaseSpanToPageCache(Span* back_span);

	// pagecache唯一的锁
	SpinLock _pagemtx;

private:

	PageCache() = default;

	PageCache(const PageCache& copy)noexcept = delete;
	PageCache& operator=(const PageCache& copy)noexcept = delete;

	PageCache(const PageCache&& copy)noexcept = delete;
	PageCache& operator=(const PageCache&& copy)noexcept = delete;

	// 唯一全局PageCache实例
	static PageCache _PageInstance;

	// 管理Span的链表
	SpanList _spList[SPAN_MAXNUM];

	// Span对象池 方便直接获取初始的Span
	SpanPool _spPool;

	// 页号和span*的映射
	RadixTree<MR_MALLOCBIT> _pgSpanHash;
};



#endif // !PAGECACHE_H

