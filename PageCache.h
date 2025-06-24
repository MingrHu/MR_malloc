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
#ifndef PAGECACHE_H
#define PAGECACHE_H
#include"Common.h"
using namespace MR_MemPoolToolKits;
// ҳ����
// �������
class PageCache {
public:

	static PageCache* getInstance() {
		return &_PageInstance;
	}

	// num�������ҳ���� ����ҳ��������ֱ���Ҷ�Ӧ��span
	Span* FetchNewSpan(size_t num);

	// ��ȡ��ַҳ�ź�span*�Ķ�Ӧ��ϵ
	Span* GetHashObjwithSpan(void* obj);

	// ��������ThreadCache��span��pagecache
	// ���Ȱ��������֯�����ɴ��span
	void ReleaseSpanToPageCache(Span* back_span);

	// pagecacheΨһ����
	SpinLock _pagemtx;

private:

	PageCache() = default;

	PageCache(const PageCache& copy)noexcept = delete;
	PageCache& operator=(const PageCache& copy)noexcept = delete;

	PageCache(const PageCache&& copy)noexcept = delete;
	PageCache& operator=(const PageCache&& copy)noexcept = delete;

	// Ψһȫ��PageCacheʵ��
	static PageCache _PageInstance;

	// ����Span������
	SpanList _spList[SPAN_MAXNUM];

	// Span����� ����ֱ�ӻ�ȡ��ʼ��Span
	SpanPool _spPool;

	// ҳ�ź�span*��ӳ��
	std::unordered_map<PAGE_ID, Span*> _pgSpanHash;
};



#endif // !PAGECACHE_H

