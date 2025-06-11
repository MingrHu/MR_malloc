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

	_freelists[pos].headpush(obj);
	if (_freelists[pos].GetRemainSize() > MAXSIZE)
		ReleaseFreeNode(pos, _freelists[pos].GetRemainSize() - MAXSIZE);
}

size_t ThreadCache::FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size){

	size_t num = CheckSize(size) > _freelists[pos].GetFreq() 
		? _freelists[pos].GetFreq(): CheckSize(size);
	CentralCache::getInstance()->FetchRangeObj(start, end, num, size, pos);
	return num;
}

void ThreadCache::ReleaseFreeNode(size_t pos, size_t num){

	void* start = _freelists[pos].GetListHead();
	void* end = nullptr;
	_freelists[pos].headRangePop(start, end, num);
	CentralCache::getInstance()->ReleaseListToSpans(start, num, pos);

}
