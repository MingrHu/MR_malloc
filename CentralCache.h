#ifndef CENTRALCACHE_H
#define CENTRALCACHE_H
#include "Common.h"
using namespace MR_MemPoolToolKits;
class CentralCache {
public:
	
	// 单例的唯一全局入口点
	static CentralCache* getInstance() {
		return &_CenInstance;
	}

	// ThreadCache拿内存块
	// size 指定大小的内存块
	// 有就直接返回 没有再向下拿
	// 这个是各个线程进行的 对应的桶需要加锁
	void FetchRangeObj(void*& start, void*& end, size_t& num, size_t size,size_t pos);

	// 从ThreadCache回收部分过长的链表
	// 如果本层也过长 则返还给下一层
	void ReleaseListToSpans(void* start, size_t num, size_t pos);
	

private:

	CentralCache() = default;
	
	// 禁用拷贝构造 赋值运算
	CentralCache(const CentralCache& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache& copy)noexcept = delete;
	// 禁用移动相关操作
	CentralCache(const CentralCache&& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache&& copy)noexcept = delete;

	// 获取一个可用的span给上层的ThreadCache
	// 如果没有则向下再申请span 然后按size划分
	// 拿到span后需要上层自己去拿span里面的内存块
	// List是指定的桶位置 size是划分的大小
	Span* GetOneSpan(SpanList& List, size_t size);

	static CentralCache _CenInstance;

	SpanList _SpanLists[FREELISTSIZE];

};
#endif // !CENTRALCACHE_H
