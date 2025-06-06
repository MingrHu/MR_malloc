#include<iostream>
#include<chrono>
#include <cstdlib>
#include<new>
#include"MR_malloc.h"
#include"Common.h"
#define SIZE 10000000
// ��ʱ��
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
	t.reset(); // ��ʼ����ʱ���Ŀ�ʼʱ��
	
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = (ListNode*)malloc(sizeof(ListNode));
		node->next = nullptr;
		free(node);
	}
	std::cout << "ϵͳ����malloc����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;

	t.reset(); // ��ʼ����ʱ���Ŀ�ʼʱ��
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = new ListNode;
		node->next = nullptr;
		delete node;
	}
	std::cout << "ϵͳ����new����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;


	MemoryPool<ListNode> mp;
	t.reset();
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = mp.Allocate();
		node->next = nullptr;
		mp.DeAllocate(node);
	}
	std::cout << "�Զ���malloc����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;

#else // ���Թ��ܿ��Ƿ�����
	const size_t Size = 1024 * 256;
	std::cout << MR_MemPoolToolKits::_GetIndexT<>::val << std::endl;

	return 0;
#endif

}