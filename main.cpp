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

	t.init(); // ��ʼ����ʱ���Ŀ�ʼʱ��
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = (ListNode*)malloc(sizeof(ListNode));
		node->next = nullptr;
		free(node);
	}
	std::cout << "ϵͳ����malloc����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;

	t.init(); // ��ʼ����ʱ���Ŀ�ʼʱ��
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = new ListNode;
		node->next = nullptr;
		delete node;
	}
	std::cout << "ϵͳ����new����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;


	MemoryPool<ListNode> mp;
	t.init();
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = mp.Allocate();
		node->next = nullptr;
		mp.DeAllocate(node);
	}
	std::cout << "�Զ���malloc����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;

#elif 0 // ���Թ��ܿ��Ƿ�����
	const size_t Size = 1024 * 256;
	for (int i = 0; i < 104; i++) 
		std::cout << MR_MemPoolToolKits::GetIndexSize(i) << std::endl;

	return 0;
#endif

}