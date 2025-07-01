#ifndef THREADCACHE_H
#define THREADCACHE_H
#include<iostream>
#include<memory>
#include "Common.h"
#include "CentralCache.h"
using namespace MR_MemPoolToolKits;
// 线程缓存
// 每个线程独享的
class ThreadCache {
public:

	ThreadCache() :_freelists() { };

	// 用户请求的内存块大小 size 
	// 对应的桶位置pos
	// 有就直接返回 没有再向下一层拿
	void* Allocate(size_t pos,size_t size);

	// 回收用户的内存块
	// 过长则返还给下一层
	void DeAllocate(void* obj, size_t pos);

private:

	// 从中心缓存中获取内存
	size_t FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size);

	// 回收链表 返还给下一层
	void ReleaseFreeNode(size_t pos,size_t num);

	_FreeLists _freelists[FREELISTSIZE];
};
// 采用智能指针自动管理线程局部变量生命周期 自动调用ThreadCache析构
static _declspec(thread) std::unique_ptr<ThreadCache> tls_threadcache = nullptr;
#endif // ! THREADCHACHE_H


