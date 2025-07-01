#ifndef THREADCACHE_H
#define THREADCACHE_H
#include<iostream>
#include<memory>
#include "Common.h"
#include "CentralCache.h"
using namespace MR_MemPoolToolKits;
// �̻߳���
// ÿ���̶߳����
class ThreadCache {
public:

	ThreadCache() :_freelists() { };

	// �û�������ڴ���С size 
	// ��Ӧ��Ͱλ��pos
	// �о�ֱ�ӷ��� û��������һ����
	void* Allocate(size_t pos,size_t size);

	// �����û����ڴ��
	// �����򷵻�����һ��
	void DeAllocate(void* obj, size_t pos);

private:

	// �����Ļ����л�ȡ�ڴ�
	size_t FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size);

	// �������� ��������һ��
	void ReleaseFreeNode(size_t pos,size_t num);

	_FreeLists _freelists[FREELISTSIZE];
};
// ��������ָ���Զ������ֲ߳̾������������� �Զ�����ThreadCache����
static _declspec(thread) std::unique_ptr<ThreadCache> tls_threadcache = nullptr;
#endif // ! THREADCHACHE_H


