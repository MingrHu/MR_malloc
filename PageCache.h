/********************************************
*  创建时间： 2025 / 05 / 16
*  名    称： 高性能内存池
*  版    本： 1.0 单线程版本
*  @author    Hu Mingrui
*  说    明： 本内存池属于第一个单线程版本 主
*  要功能是为了给频繁请求和释放内存的线程提供
*  更高效的操作 主要原理是缓存了内存 且对申请
*  的块按一定规则进行内存对齐 减少了内存外碎片
*  的产生 后续本项目会持续升级更新
* *******************************************
*  修改时间： 2025 / 05 / 23
*  版    本： 1.1 单线程版本
*  改动说明：
********************************************/
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
	std::unordered_map<PAGE_ID, Span*> _pgSpanHash;
};



#endif // !PAGECACHE_H

