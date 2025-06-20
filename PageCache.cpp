#include "PageCache.h"

PageCache PageCache::_PageInstance;

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
    // 当前这个桶没有 去大于num的桶找
    for (size_t i = num; i < SPAN_MAXNUM; i++) {
        if (_spList[i].Begin()) {

            newSpan = _spList[i].headPopSpan();
            newSpan->_pageNum = num;

            Span* spright = _spPool._spAllocate();
            spright->_pageNum = i - num + 1;
            spright->_pageID = newSpan->_pageID + num;
            for (size_t i = 0; i < newSpan->_pageNum; i++)
                _pgSpanHash[newSpan->_pageID + i] = newSpan;

            // 标记一下 方便后续合并
            _pgSpanHash[spright->_pageID] = spright;
            _pgSpanHash[spright->_pageID + spright->_pageNum - 1] = spright;
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

    assert(obj);
    PAGE_ID pgid = reinterpret_cast<PAGE_ID>(obj) >> PAGE_SHIFT;
    if (_pgSpanHash.find(pgid) != _pgSpanHash.end())
        return _pgSpanHash[pgid];
    return nullptr;
}

void PageCache::ReleaseSpanToPageCache(Span* back_span){

    assert(back_span);
    PAGE_ID pgid = back_span->_pageID;
    PAGE_ID pgnum = back_span->_pageNum;
    if (pgnum > SPAN_MAXNUM) {
        SystemFree(pgid);
        _spPool._spDellocate(back_span);
        return;
    }
    // 开始接手管理spanlist链表合并操作
    // 分为向前找页和向后找页
    // 找到的页必须是一个span管理的
    PAGE_ID start_pgid = pgid - 1;
    PAGE_ID page_sum = pgnum;
    while (1) {
        // 向前找必须是在当前的start_pgid上 -1
        if (_pgSpanHash.find(start_pgid) == _pgSpanHash.end()) {
            start_pgid += 1;
            break;
        }    
        Span* sp = _pgSpanHash[start_pgid];
        if (sp->_isUse || sp->_pageNum + pgnum > SPAN_MAXNUM) {
            start_pgid += 1;
            break;
        }
       
        _spList[sp->_pageNum - 1].Erase(sp);
        pgnum += sp->_pageNum;
        start_pgid = sp->_pageID - 1;
        
        _spPool._spDellocate(sp);
    }
    // 从当前位置往后找 next_pgid一定是下一个span的起始位置
    PAGE_ID next_pgid = pgid + back_span->_pageNum;
    // 向后查找
    while (1) {
        if (_pgSpanHash.find(next_pgid) == _pgSpanHash.end()) 
            break;
        Span* sp = _pgSpanHash[next_pgid];
        if (sp->_isUse || sp->_pageNum + pgnum > SPAN_MAXNUM) 
            break;
        // 递增
        pgnum += sp->_pageNum;
        next_pgid += sp->_pageNum;
        // 合并前删除链表上的sp结点
        // 同时回收内存
        _spList[sp->_pageNum - 1].Erase(sp);
        _spPool._spDellocate(sp);
        
    }
    // 无需删除原有映射关系 各个Span是相隔的
    // 主要是因为合并后的span是一个整体的
    // 在后续合并直接从两端出发寻找
    // 更新关系并开始合并
    back_span->_pageID = start_pgid;
    back_span->_pageNum = pgnum;

    _pgSpanHash[back_span->_pageID] = back_span;
    _pgSpanHash[back_span->_pageID + pgnum - 1] = back_span;
    _spList[pgnum - 1].headPushSpan(back_span);

}
