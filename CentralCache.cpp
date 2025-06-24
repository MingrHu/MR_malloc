#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_CenInstance;

void CentralCache::FetchRangeObj(void*& start, void*& end, size_t& num, size_t size,size_t pos){
    
    // ����
    _SpanLists[pos]._mtx.Lock();
    // ȥ������span �õ��˾�ֱ�ӷ��� û�õ������²�����
    Span* span = GetOneSpan(_SpanLists[pos], size);
    start = span->_freelist.GetListHead();
    span->_freelist.headRangePop(start,end, num);
    span->_usedcount += num;

    _SpanLists[pos]._mtx.Unlock();

}


Span* CentralCache::GetOneSpan(SpanList& List, size_t size){
    
    Span* res = List.Begin();
    while (res) {
        if (res->_freelist.GetRemainSize())
            return res;
        res = res->_next;
    }
    // �������ǽ�������
    // List._mtx.unlock();
    // �����ҳ������
    size_t num = CheckPageNum(size);
    // ���²���span �зֺ������spanlist��
    // ��Ҫ��PageCache���м�������
    // ��֤�õ���span���޸�Ϊʹ��״̬
    // ���������߳̿������ڶ����span���кϲ�
    // ������span���ڴ���������
    PageCache::getInstance()->_pagemtx.Lock();
    Span* newSpan = PageCache::getInstance()->FetchNewSpan(num);
    newSpan->_usedcount = 0;
    newSpan->_isUse = true;     
    PageCache::getInstance()->_pagemtx.Unlock();
    

    // reinterpret_cast �ȼ���(char*) �ײ�����תָ��
    char* start = reinterpret_cast<char*>(newSpan->_pageID << PAGE_SHIFT);
    char* chunk = start;
    // ��ʼ�з�
    while (chunk + size <= start + (newSpan->_pageNum << PAGE_SHIFT)) {
        newSpan->_freelist.headpush(chunk);
        newSpan->_sum += 1;
        chunk += size;
    }
    List.headPushSpan(newSpan);
    return newSpan;
}

void CentralCache::ReleaseListToSpans(void* start,size_t pos){

    _SpanLists[pos]._mtx.Lock();
    while (start) {
        // �����ڴ��ַ�õ�������span����
        // ����ʹ���˹�ϣ�� ����̲߳���ȫ Ҫ����
        PageCache::getInstance()->_pagemtx.Lock();
        Span* span = PageCache::getInstance()->GetHashObjwithSpan(start);
        PageCache::getInstance()->_pagemtx.Unlock();
        assert(span);

        void* next = *(void**)start;
        span->_usedcount -= 1;
        span->_freelist.headpush(start);
        // ������span�����ж� 
        // �����Ӧ��span����ʹ�õ�����Ϊ0 ����Է�����pagecache
        if (span->_usedcount == 0) {
            // �ͷŽ��
            _SpanLists[pos].Erase(span);
            span->_freelist.InitFreeLists();
            PageCache::getInstance()->_pagemtx.Lock();
            span->_isUse = false;
            PageCache::getInstance()->ReleaseSpanToPageCache(span);
            PageCache::getInstance()->_pagemtx.Unlock();
        }
        start = next;
    }

    _SpanLists[pos]._mtx.Unlock();
}
