#include "ThreadCache.h"

void* ThreadCache::Allocate(size_t pos,size_t size){

	if (_freelists[pos].Empty()) {
		void* start = nullptr;
		void* end = nullptr;
		size_t num = FetchFromCentralCache(start,end,pos, size);
		_freelists[pos].headRangePush(start, end, num);
	}	
	return _freelists[pos].headpop();
}

void ThreadCache::DeAllocate(void* obj, size_t pos){

	// printf("Start to Cycle ThreadCache!");
	_freelists[pos].headpush(obj);
	// 回收策略待定
	if (_freelists[pos].GetRemainSize() > MAXSIZE) 
		ReleaseFreeNode(pos, _freelists[pos].GetRemainSize());
}


size_t ThreadCache::FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size){

	size_t num = CheckSize(size) > _freelists[pos].GetFreq() 
		? _freelists[pos].GetFreq(): CheckSize(size);
	CentralCache::getInstance()->FetchRangeObj(start, end, num, size, pos);
	// 向下一层申请内存就增加freq
	_freelists[pos].PlusFreq();
	return num;
}

void ThreadCache::ReleaseFreeNode(size_t pos, size_t num){

	void* start = _freelists[pos].GetListHead();
	void* end = nullptr;
	_freelists[pos].headRangePop(start, end, num);
	CentralCache::getInstance()->ReleaseListToSpans(start, pos);
	// 向下一层还就减少
	_freelists[pos].SubFreq();
}
