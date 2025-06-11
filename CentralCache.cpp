#include "CentralCache.h"
#include "PageCache.h"
void CentralCache::FetchRangeObj(void*& start, void*& end, size_t& num, size_t size,size_t pos){
    
    // 加锁
    _SpanLists[pos]._mtx.lock();
    // 去申请拿span 这个过程涉及到解锁加锁的问题
    Span* span = GetOneSpan(_SpanLists[pos], size);
    
    start = span->_freelist.GetListHead();
    span->_freelist.headRangePop(start,end, num);
    span->_usedcount -= num;

    _SpanLists[pos]._mtx.unlock();

}

Span* CentralCache::GetOneSpan(SpanList& List, size_t size){
    
    Span* res = List.Begin();
    while (res) {
        if (!res->_freelist.Empty())
            return res;
        res = res->_next;
    }
    
    // 后续考虑加锁
    // List._mtx.unlock();
    size_t num = 2; 
    Span* newSpan = PageCache::getInstance()->FetchNewSpan(num);
    newSpan->_usedcount = 0;
    newSpan->_pageID << PAGE_SHIFT;


    

    
}

void CentralCache::ReleaseListToSpans(void* start, size_t size,size_t pos){
    return;
}
