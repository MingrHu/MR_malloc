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

// ntimes 一轮申请和释放内存的次数
// rounds 轮次
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds) {
	vector<thread> vthread(nworks);
	atomic<size_t> malloc_costtime = 0;
	atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k) {
		// 立即开始执行
		vthread[k] = thread([&, k]() {
			vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j) {
				auto begin1 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					v.push_back(malloc(16));
					// v.puhs_back(malloc(i%(256*1024)));
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

	printf("系统自带malloc策略：%zu个线程并发执行%zu轮，每轮申请空间 %zu次: 花费：%zu ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("系统自带malloc策略： %zu个线程并发执行%zu轮次，每轮释放空间 %zu次: 花费：%zu ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("系统自带malloc策略：%zu个线程并发malloc&free %zu次，总计花费：%zu ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

// ntimes 一轮申请和释放内存的次数
// rounds 轮次
// 单轮次申请释放次数 线程数 轮次
void BenchmarkMR_malloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;
	MR_malloc mp;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = thread([&, k]() {
			vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j) {
				auto begin1 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					v.push_back(malloc(16));
					//v.push_back(mp.Allocate(i % (256 * 1024)));
				}
				auto end1 = chrono::high_resolution_clock::now();

				auto begin2 = chrono::high_resolution_clock::now();
				for (size_t i = 0; i < ntimes; i++) {
					v.push_back(malloc(16));
					//mp.Dellocate(v[i], i % (257 * 1024));
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

	printf("采用MR_malloc策略：%u个线程并发执行%u轮，每轮申请空间 %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("采用MR_malloc策略：%u个线程并发执行%u轮，每轮释放空间 %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("采用MR_malloc策略：%u个线程并发%u次，总计花费：%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

int main() {
#if 0
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


	MR_malloc<ListNode> mp;
	t.init();
	for (int i = 0; i < SIZE; i++) {
		ListNode* node = mp.Allocate();
		node->next = nullptr;
		mp.Dellocate(node);
	}
	std::cout << "自定义malloc分配" << SIZE << "次内存所用时间为：" << t.elapsed() << "ms" << std::endl;

#elif 0 // 测试功能块是否正常
	const size_t Size = 1024 * 256;
	//for (int i = 1; i <= Size; i++) 
	std::cout << MR_malloc::_CalRoundUp(256*1024-1).second<< std::endl;

	
#elif 1

	size_t n = 500;
	cout << "==========================================================" << endl;
	BenchmarkMalloc(n, 4, 10000);

	cout << "==========================================================" << endl;
	BenchmarkMR_malloc(n, 4, 10000);
	cout << endl << endl;

#endif
	return 0;
}