#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_CenInstance;

void CentralCache::FetchRangeObj(void*& start, void*& end, size_t& num, size_t size,size_t pos){
    
    // 加锁
    _SpanLists[pos]._mtx.lock();
    // 去申请拿span 拿到了就直接返回 没拿到就向下层申请
    Span* span = GetOneSpan(_SpanLists[pos], size);
    
    start = span->_freelist.GetListHead();
    span->_freelist.headRangePop(start,end, num);
    span->_usedcount += num;

    _SpanLists[pos]._mtx.unlock();

}

Span* CentralCache::GetOneSpan(SpanList& List, size_t size){
    
    Span* res = List.Begin();
    while (res) {
        if (!res->_freelist.Empty())
            return res;
        res = res->_next;
    }
    
    // 后续考虑解锁问题
    // List._mtx.unlock();

    size_t num = 2; 
    // 向下层拿span 切分后挂载至spanlist上
    // 需要对PageCache进行加锁操作
    PageCache::getInstance()->_pagemtx.lock();
    Span* newSpan = PageCache::getInstance()->FetchNewSpan(num);
    PageCache::getInstance()->_pagemtx.unlock();
    newSpan->_usedcount = 0;
    newSpan->_isUse = true;

    // reinterpret_cast 等价于（char*) 底层整数转指针
    char* start = reinterpret_cast<char*>(newSpan->_pageID << PAGE_SHIFT);
    char* chunk = start;
    // 开始切分
    while (chunk < start + (newSpan->_pageNum << PAGE_SHIFT)) {
        newSpan->_freelist.headpush(chunk);
        chunk += size;
    }

    List.headPushSpan(newSpan);
    return newSpan;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size,size_t pos){

    _SpanLists[pos]._mtx.lock();
    while (start) {
        // 根据内存地址得到所属的span对象
        // 由于使用了哈希表 因此线程不安全 要加锁
        PageCache::getInstance()->_pagemtx.lock();
        Span* span = PageCache::getInstance()->GetHashObjwithSpan(start);
        PageCache::getInstance()->_pagemtx.unlock();

        void* next = *(void**)start;
        span->_usedcount -= 1;
        span->_freelist.headpush(start);
        // 返还至span进行判断 
        // 如果对应的span里面使用的数量为0 则可以返还至pagecache
        if (span->_usedcount == 0) {
            _SpanLists[pos].Erase(span);
            PageCache::getInstance()->_pagemtx.lock();
            span->_isUse = false;
            PageCache::getInstance()->ReleaseSpanToPageCache(span);
            PageCache::getInstance()->_pagemtx.unlock();
        }
        start = next;
    }

    _SpanLists[pos]._mtx.unlock();
}
