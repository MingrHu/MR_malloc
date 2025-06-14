#include "PageCache.h"

Span* PageCache::FetchNewSpan(size_t num){   
    
    if (num > SPAN_MAXNUM) {
        void* pgmem_start = SystemAlloc(num);
        Span* res = _spPool._spAllocate();
        res->_pageID = reinterpret_cast<PAGE_ID>(pgmem_start) >> PAGE_SHIFT;
        res->_pageNum = num;
        for (int i = 0; i < num; i++)
            _pgSpanHash[res->_pageID + i] = res;
    }
    // 给可能找到的span准备
    Span* newSpan = nullptr;
    // 当前这个桶有
    if (_spList[num - 1].Begin()) {
        newSpan = _spList[num - 1].headPopSpan();
        PAGE_ID pgid = newSpan->_pageID;
        PAGE_ID pgnum = newSpan->_pageNum;
        for (int i = 0; i < pgnum; i++)
            _pgSpanHash[pgid + i] = newSpan;
        return newSpan;
    }
    // 当前这个桶没有
    for (int i = num; i < SPAN_MAXNUM; i++) {
        if (_spList[i].Begin()) {

            newSpan = _spList[i].headPopSpan();
            newSpan->_pageNum = num;

            Span* spright = _spPool._spAllocate();
            spright->_pageNum = i - num + 1;
            spright->_pageID = newSpan->_pageID + spright->_pageNum;
            for (int i = 0; i < spright->_pageNum; i++)
                _pgSpanHash[spright->_pageID + i] = spright;

            _spList[spright->_pageNum - 1].headPushSpan(spright);
            return newSpan;
        }
    }
    // 找到这说明没有桶满足请求的num页个数
    // 必须自己开辟空间了
    Span* res = _spPool._spAllocate();
    void* pgmem_start = SystemAlloc(SPAN_MAXNUM);
    // 获取起始页号
    res->_pageID = reinterpret_cast<PAGE_ID>(pgmem_start) >> PAGE_SHIFT;
    res->_pageNum = SPAN_MAXNUM;
    _spList[SPAN_MAXNUM - 1].headPushSpan(res);
    
    return FetchNewSpan(num);
}

Span* PageCache::GetHashObjwithSpan(void* obj){

    PAGE_ID pgid = reinterpret_cast<PAGE_ID>(obj) >> PAGE_SHIFT;
    if (_pgSpanHash.find(pgid) != _pgSpanHash.end())
        return _pgSpanHash[pgid];
    return nullptr;
}

void PageCache::ReleaseSpanToPageCache(Span* back_span){



}
