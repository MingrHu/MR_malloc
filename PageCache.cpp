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
    // �������ҵ���span׼��
    Span* newSpan = nullptr;
    // ��ǰ���Ͱ��
    if (_spList[num - 1].Begin()) {
        newSpan = _spList[num - 1].headPopSpan();
        PAGE_ID pgid = newSpan->_pageID;
        PAGE_ID pgnum = newSpan->_pageNum;
        
        for (int i = 0; i < pgnum; i++)
            _pgSpanHash[pgid + i] = newSpan;
        return newSpan;
    }
    // ��ǰ���Ͱû�� ȥ����num��Ͱ��
    for (size_t i = num; i < SPAN_MAXNUM; i++) {
        if (_spList[i].Begin()) {

            newSpan = _spList[i].headPopSpan();
            newSpan->_pageNum = num;

            Span* spright = _spPool._spAllocate();
            spright->_pageNum = i - num + 1;
            spright->_pageID = newSpan->_pageID + num;
            for (size_t i = 0; i < newSpan->_pageNum; i++)
                _pgSpanHash[newSpan->_pageID + i] = newSpan;

            // ���һ�� ��������ϲ�
            _pgSpanHash[spright->_pageID] = spright;
            _pgSpanHash[spright->_pageID + spright->_pageNum - 1] = spright;
            _spList[spright->_pageNum - 1].headPushSpan(spright);
            return newSpan;
        }
    }
    // �ҵ���˵��û��Ͱ���������numҳ����
    // �����Լ����ٿռ���
    Span* res = _spPool._spAllocate();
    void* pgmem_start = SystemAlloc(SPAN_MAXNUM);
    // ��ȡ��ʼҳ��
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
    // ��ʼ���ֹ���spanlist����ϲ�����
    // ��Ϊ��ǰ��ҳ�������ҳ
    // �ҵ���ҳ������һ��span�����
    PAGE_ID start_pgid = pgid - 1;
    PAGE_ID page_sum = pgnum;
    while (1) {
        // ��ǰ�ұ������ڵ�ǰ��start_pgid�� -1
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
    // �ӵ�ǰλ�������� next_pgidһ������һ��span����ʼλ��
    PAGE_ID next_pgid = pgid + back_span->_pageNum;
    // ������
    while (1) {
        if (_pgSpanHash.find(next_pgid) == _pgSpanHash.end()) 
            break;
        Span* sp = _pgSpanHash[next_pgid];
        if (sp->_isUse || sp->_pageNum + pgnum > SPAN_MAXNUM) 
            break;
        // ����
        pgnum += sp->_pageNum;
        next_pgid += sp->_pageNum;
        // �ϲ�ǰɾ�������ϵ�sp���
        // ͬʱ�����ڴ�
        _spList[sp->_pageNum - 1].Erase(sp);
        _spPool._spDellocate(sp);
        
    }
    // ����ɾ��ԭ��ӳ���ϵ ����Span�������
    // ��Ҫ����Ϊ�ϲ����span��һ�������
    // �ں����ϲ�ֱ�Ӵ����˳���Ѱ��
    // ���¹�ϵ����ʼ�ϲ�
    back_span->_pageID = start_pgid;
    back_span->_pageNum = pgnum;

    _pgSpanHash[back_span->_pageID] = back_span;
    _pgSpanHash[back_span->_pageID + pgnum - 1] = back_span;
    _spList[pgnum - 1].headPushSpan(back_span);

}
