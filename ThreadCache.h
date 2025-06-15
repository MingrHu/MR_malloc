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
#ifndef THREADCACHE_H
#define THREADCACHE_H
#include<iostream>
#include<type_traits>
#include<vector>
#include "Common.h"
#include "CentralCache.h"
using namespace MR_MemPoolToolKits;
// 线程缓存
// 每个线程独享的
class ThreadCache {
public:

	// 用户请求的内存块大小 size 
	// 对应的桶位置pos
	// 有就直接返回 没有再向下一层拿
	void* Allocate(size_t pos,size_t size);

	// 回收用户的内存块
	// 过长则返还给下一层
	void DeAllocate(void* obj, size_t pos);

	// 各个线程的内存回收工作
	// 如果对应线程的ThreadCache销毁 需要还内存
	~ThreadCache();

private:

	// 从中心缓存中获取内存
	size_t FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size);

	// 回收链表 返还给下一层
	void ReleaseFreeNode(size_t pos,size_t num);

	_FreeLists _freelists[FREELISTSIZE];
};
static _declspec(thread) ThreadCache tls_threadcache;
#endif // ! THREADCHACHE_H


