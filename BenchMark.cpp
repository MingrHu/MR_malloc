#include<iostream>
#include<chrono>
#include<cstdlib>
#include<new>
#include <atomic>
#include <iomanip> 
#include <fstream>
#include"Common.h"
#include"MR_malloc.h"
#define SIZE 10000000
typedef long long ll;
using namespace std;
struct ListNode {
	int val;
	ListNode* next;
};

// ntimes 一轮申请和释放内存的次数
// rounds 轮次
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds,vector<int>& test) {
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
					//v.push_back(malloc(15));
					v.push_back(malloc(test[i]));
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

	printf("系统自带malloc策略：%zu个线程并发执行%zu轮次，每轮释放空间 %zu次: 花费：%zu ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("系统自带malloc策略：%zu个线程并发malloc&free %zu次，总计花费：%zu ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

// ntimes 一轮申请和释放内存的次数
// rounds 轮次
// 单轮次申请释放次数 线程数 轮次
void BenchmarkMR_malloc(size_t ntimes, size_t nworks, size_t rounds, vector<int>& test)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;
	MR_malloc mp;

	// 每条线程
	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = thread([&, k]() {
			vector<void*> v;
			v.reserve(ntimes);
			// 每轮
			for (size_t j = 0; j < rounds; ++j) {
				auto begin1 = chrono::high_resolution_clock::now();
				// 每次分配
				for (size_t i = 0; i < ntimes; i++) {

					v.push_back(mp.Allocate(test[i]));
				}
				auto end1 = chrono::high_resolution_clock::now();

				auto begin2 = chrono::high_resolution_clock::now();
				// 每次释放
				for (size_t i = 0; i < ntimes; i++) {
					mp.Dellocate(v[i], test[i]);
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
	unordered_map<int, int> dic;
	for (int i = 1; i <= Size; i++) {
		size_t t = MR_MemPoolToolKits::CheckPageNum(i);
		dic[t] += 1;
		std::cout << t << std::endl;
	}
	cout << dic.size() << endl;
#elif 0

	double MAX1 = 0, MAX2 = 0, size1 = 0, size2 = 0, i1 = 0, i2 = 0;
	MR_malloc mp;
	ofstream outFile("TestReport/memory_wastage_report.txt");
	if (!outFile) {
		cerr << "无法打开输出文件！" << endl;
		return 1;
	}
	outFile << "请求大小" << "\t" << "分配大小" << "\t" << "外部碎片率" << "\t" << "内部碎片率" << "\t" << "分配页个数" << endl;
	for (int i = 1; i <= 256 * 1024; i++) {
		size_t size = mp._CalRoundUp(i).first;
		double per1 = ((double)size - (double)i) / (double)size * 100;
		outFile << i<<"\t" << size << "\t";
		outFile << fixed << setprecision(2);
		outFile << per1 << "%" << "\t";
		size_t pagenum = MR_MemPoolToolKits::CheckPageNum(size);
		double per2 = ((double)(pagenum * 4096 % size) / (double)(pagenum * 4096)) * 100;
		outFile << per2 << "%" <<"\t"<<pagenum<< endl;
		if (MAX1 < per1)
			MAX1 = per1, size1 = size, i1 = i;
		if (MAX2 < per2)
			MAX2 = per2, size2 = size, i2 = i;
	}
	cout << "最大外部碎片率为：" << MAX1 << "%" << " 最大内部碎片率为：" << MAX2 << "%" << endl;
	cout << "对应分配申请大小和实际大小分别为：" << i1 << "-"<<size1<<" " << i2 << "-" << size2<< endl;
	return 0;


	
#elif 1
	size_t n = 256*1024 + 1;
	vector<int> nums(n, 0);
	for (int i = 1; i < n; i++) {
		nums[i] = rand() % i;
	}
	cout << "==========================================================" << endl;
	BenchmarkMalloc(n, 8, 1,nums);

	cout << "==========================================================" << endl;
	BenchmarkMR_malloc(n, 8, 1,nums);

#endif
	return 0;
}