/********************************************
*  ����ʱ�䣺 2025 / 05 / 16
*  ��    �ƣ� �������ڴ��
*  ��    ���� 1.0 ���̰߳汾
*  @author    Hu Mingrui
*  ˵    ���� ���ڴ�����ڵ�һ�����̰߳汾 ��
*  Ҫ������Ϊ�˸�Ƶ��������ͷ��ڴ���߳��ṩ
*  ����Ч�Ĳ��� ��Ҫԭ���ǻ������ڴ� �Ҷ�����
*  �Ŀ鰴һ����������ڴ���� �������ڴ�����Ƭ
*  �Ĳ��� ��������Ŀ�������������
* *******************************************
*  �޸�ʱ�䣺 2025 / 05 / 23
*  ��    ���� 1.1 ���̰߳汾
*  �Ķ�˵����
********************************************/
#ifndef THREADCACHE_H
#define THREADCACHE_H
#include<iostream>
#include<type_traits>
#include<vector>
#include "Common.h"
#include "CentralCache.h"
using namespace MR_MemPoolToolKits;
// �̻߳���
// ÿ���̶߳����
class ThreadCache {
public:

	// �û�������ڴ���С size 
	// ��Ӧ��Ͱλ��pos
	// �о�ֱ�ӷ��� û��������һ����
	void* Allocate(size_t pos,size_t size);

	// �����û����ڴ��
	// �����򷵻�����һ��
	void DeAllocate(void* obj, size_t pos);

	// �����̵߳��ڴ���չ���
	// �����Ӧ�̵߳�ThreadCache���� ��Ҫ���ڴ�
	~ThreadCache();

private:

	// �����Ļ����л�ȡ�ڴ�
	size_t FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size);

	// �������� ��������һ��
	void ReleaseFreeNode(size_t pos,size_t num);

	_FreeLists _freelists[FREELISTSIZE];
};
static _declspec(thread) ThreadCache tls_threadcache;
#endif // ! THREADCHACHE_H


