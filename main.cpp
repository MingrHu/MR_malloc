#include<iostream>
#include<chrono>
#include<cstdlib>
#include<new>
#include"ThreadCache.h"
#include"Common.h"
#define SIZE 10000000

struct ListNode {
	int val;
	ListNode* next;
};

int main() {
#if 1
	Timer t;

	t.init(); // 初始化计时器的开始时间
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = (ListNode*)malloc(sizeof(ListNode));
		node->next = nullptr;
		free(node);
	}
	std::cout << "系统调用malloc分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;

	t.init(); // 初始化计时器的开始时间
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = new ListNode;
		node->next = nullptr;
		delete node;
	}
	std::cout << "系统调用new分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;


	MemoryPool<ListNode> mp;
	t.init();
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = mp.Allocate();
		node->next = nullptr;
		mp.DeAllocate(node);
	}
	std::cout << "自定义malloc分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;

#elif 0 // 测试功能块是否正常
	const size_t Size = 1024 * 256;
	for (int i = 0; i < 104; i++) 
		std::cout << MR_MemPoolToolKits::GetIndexSize(i) << std::endl;

	return 0;
#endif

}