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
	if (_freelists[pos].GetRemainSize() > MAXSIZE)
		ReleaseFreeNode(pos, _freelists[pos].GetRemainSize() - MAXSIZE);
}


size_t ThreadCache::FetchFromCentralCache(void*& start,void*& end,size_t pos, size_t size){

	size_t num = CheckSize(size) > _freelists[pos].GetFreq() 
		? _freelists[pos].GetFreq(): CheckSize(size);
	CentralCache::getInstance()->FetchRangeObj(start, end, num, size, pos);
	_freelists[pos].PlusFreq();
	return num;
}

void ThreadCache::ReleaseFreeNode(size_t pos, size_t num){

	void* start = _freelists[pos].GetListHead();
	void* end = nullptr;
	_freelists[pos].headRangePop(start, end, num);
	CentralCache::getInstance()->ReleaseListToSpans(start, pos);
	_freelists[pos].SubFreq();
}

ThreadCache::~ThreadCache(){
	// test
	// printf("Start to Destroy ThreadCache!");
	for (size_t i = 0; i < FREELISTSIZE; i++) {
		if (!_freelists[i].Empty()) {
			size_t num = _freelists[i].GetRemainSize();
			void* start = nullptr;
			void* end = nullptr;
			_freelists[i].headRangePop(start, end, num);
			CentralCache::getInstance()->ReleaseListToSpans(start, i);
		}
	}
}
