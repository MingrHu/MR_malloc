#include<iostream>
#include<chrono>
#include<cstdlib>
#include<new>
#include <atomic>
#include"Common.h"
#include"MR_malloc.h"
#define SIZE 10000000
using namespace std;
struct ListNode {
	int val;
	ListNode* next;
};

// ntimes һ��������ͷ��ڴ�Ĵ���
// rounds �ִ�
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds) {
	vector<thread> vthread(nworks);
	atomic<size_t> malloc_costtime = 0;
	atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k) {
		// ������ʼִ��
		vthread[k] = thread([&, k]() {
			vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j) {
				auto begin1 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					v.push_back(malloc(16));
					// v.puhs_back(malloc(i%256*1024));
				}
				auto end1 = chrono::high_resolution_clock::now();

				auto begin2 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					free(v[i]);
				}
				auto end2 = chrono::high_resolution_clock::now();
				v.clear();

				auto duration1 = chrono::duration_cast<chrono::milliseconds>(end1 - begin1).count();
				auto duration2 = chrono::duration_cast<chrono::milliseconds>(end2 - begin2).count();

				malloc_costtime += duration1;
				free_costtime += duration2;
			}
			});
	}

	for (auto& t : vthread) {
		t.join();
	}

	printf("%zu���̲߳���ִ��%zu�ִΣ�ÿ�ִ�malloc %zu��: ���ѣ�%zu ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%zu���̲߳���ִ��%zu�ִΣ�ÿ�ִ�free %zu��: ���ѣ�%zu ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%zu���̲߳���malloc&free %zu�Σ��ܼƻ��ѣ�%zu ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

// ntimes һ��������ͷ��ڴ�Ĵ���
// rounds �ִ�
// ���ִ������ͷŴ��� �߳��� �ִ�
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;
	MR_malloc<ListNode> mp;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = thread([&, k]() {
			vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j) {
				auto begin1 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					v.push_back();
				}
				auto end1 = chrono::high_resolution_clock::now();

				auto begin2 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					free(v[i]);
				}
				auto end2 = chrono::high_resolution_clock::now();
				v.clear();

				auto duration1 = chrono::duration_cast<chrono::milliseconds>(end1 - begin1).count();
				auto duration2 = chrono::duration_cast<chrono::milliseconds>(end2 - begin2).count();

				malloc_costtime += duration1;
				free_costtime += duration2;
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent alloc %u��: ���ѣ�%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent dealloc %u��: ���ѣ�%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%u���̲߳���concurrent alloc&dealloc %u�Σ��ܼƻ��ѣ�%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

int main()
{
	size_t n = 50;
	cout << "==========================================================" << endl;
	BenchmarkConcurrentMalloc(n, 4, 10000);
	cout << endl << endl;

	BenchmarkMalloc(n, 4, 10000);
	cout << "==========================================================" << endl;

	return 0;
}

int main() {
#if 0
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


	MR_malloc<ListNode> mp;
	t.init();
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = mp.Allocate();
		node->next = nullptr;
		mp.Dellocate(node);
	}
	std::cout << "�Զ���malloc����" << SIZE << "���ڴ�����ʱ��Ϊ��" << t.elapsed() << "ms" << std::endl;

#elif 0 // ���Թ��ܿ��Ƿ�����
	const size_t Size = 1024 * 256;
	for (int i = 0; i < 104; i++) 
		std::cout << MR_MemPoolToolKits::GetIndexSize(i) << std::endl;

	return 0;
#elif 1

#endif

}