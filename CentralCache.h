#ifndef CENTRALCACHE_H
#define CENTRALCACHE_H
#include "Common.h"
using namespace MR_MemPoolToolKits;
class CentralCache {
public:
	
	// ������Ψһȫ����ڵ�
	static CentralCache* getInstance() {
		return &_CenInstance;
	}

	// ThreadCache���ڴ��
	// size ָ����С���ڴ��
	// �о�ֱ�ӷ��� û����������
	// ����Ǹ����߳̽��е� ��Ӧ��Ͱ��Ҫ����
	void FetchRangeObj(void*& start, void*& end, size_t& num, size_t size,size_t pos);

	// ��ThreadCache���ղ��ֹ���������
	// �������Ҳ���� �򷵻�����һ��
	void ReleaseListToSpans(void* start, size_t num, size_t pos);
	

private:

	CentralCache() = default;
	
	// ���ÿ������� ��ֵ����
	CentralCache(const CentralCache& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache& copy)noexcept = delete;
	// �����ƶ���ز���
	CentralCache(const CentralCache&& copy)noexcept = delete;
	CentralCache& operator=(const CentralCache&& copy)noexcept = delete;

	// ��ȡһ�����õ�span���ϲ��ThreadCache
	// ���û��������������span Ȼ��size����
	// �õ�span����Ҫ�ϲ��Լ�ȥ��span������ڴ��
	// List��ָ����Ͱλ�� size�ǻ��ֵĴ�С
	Span* GetOneSpan(SpanList& List, size_t size);

	static CentralCache _CenInstance;

	SpanList _SpanLists[FREELISTSIZE];

};
#endif // !CENTRALCACHE_H
