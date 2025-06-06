#include<iostream>
#include<chrono>
#include <cstdlib>
#include<new>
#include"MR_malloc.h"
#include"Common.h"
#define SIZE 10000000
// 计时器
class Timer {
public:
	Timer() : start_(std::chrono::high_resolution_clock::now()) {}

	void reset() {
		start_ = std::chrono::high_resolution_clock::now();
	}

	double elapsed() const {
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
		return duration.count();
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

struct ListNode {
	int val;
	ListNode* next;
};

int main() {
#if 1
	Timer t;
	t.reset(); // 初始化计时器的开始时间
	
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = (ListNode*)malloc(sizeof(ListNode));
		node->next = nullptr;
		free(node);
	}
	std::cout << "系统调用malloc分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;

	t.reset(); // 初始化计时器的开始时间
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = new ListNode;
		node->next = nullptr;
		delete node;
	}
	std::cout << "系统调用new分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;


	MemoryPool<ListNode> mp;
	t.reset();
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = mp.Allocate();
		node->next = nullptr;
		mp.DeAllocate(node);
	}
	std::cout << "自定义malloc分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;

#else // 测试功能块是否正常
	const size_t Size = 1024 * 256;
	std::cout << MR_MemPoolToolKits::_GetIndexT<>::val << std::endl;

	return 0;
#endif

}