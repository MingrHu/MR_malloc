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

// ntimes һ��������ͷ��ڴ�Ĵ���
// rounds �ִ�
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds,vector<int>& test) {
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

	printf("ϵͳ�Դ�malloc���ԣ�%zu���̲߳���ִ��%zu�֣�ÿ������ռ� %zu��: ���ѣ�%zu ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("ϵͳ�Դ�malloc���ԣ�%zu���̲߳���ִ��%zu�ִΣ�ÿ���ͷſռ� %zu��: ���ѣ�%zu ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("ϵͳ�Դ�malloc���ԣ�%zu���̲߳���malloc&free %zu�Σ��ܼƻ��ѣ�%zu ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

// ntimes һ��������ͷ��ڴ�Ĵ���
// rounds �ִ�
// ���ִ������ͷŴ��� �߳��� �ִ�
void BenchmarkMR_malloc(size_t ntimes, size_t nworks, size_t rounds, vector<int>& test)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;
	MR_malloc mp;

	// ÿ���߳�
	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = thread([&, k]() {
			vector<void*> v;
			v.reserve(ntimes);
			// ÿ��
			for (size_t j = 0; j < rounds; ++j) {
				auto begin1 = chrono::high_resolution_clock::now();
				// ÿ�η���
				for (size_t i = 0; i < ntimes; i++) {

					v.push_back(mp.Allocate(test[i]));
				}
				auto end1 = chrono::high_resolution_clock::now();

				auto begin2 = chrono::high_resolution_clock::now();
				// ÿ���ͷ�
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

	printf("����MR_malloc���ԣ�%u���̲߳���ִ��%u�֣�ÿ������ռ� %u��: ���ѣ�%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("����MR_malloc���ԣ�%u���̲߳���ִ��%u�֣�ÿ���ͷſռ� %u��: ���ѣ�%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("����MR_malloc���ԣ�%u���̲߳���%u�Σ��ܼƻ��ѣ�%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
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
		cerr << "�޷�������ļ���" << endl;
		return 1;
	}
	outFile << "�����С" << "\t" << "�����С" << "\t" << "�ⲿ��Ƭ��" << "\t" << "�ڲ���Ƭ��" << "\t" << "����ҳ����" << endl;
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
	cout << "����ⲿ��Ƭ��Ϊ��" << MAX1 << "%" << " ����ڲ���Ƭ��Ϊ��" << MAX2 << "%" << endl;
	cout << "��Ӧ���������С��ʵ�ʴ�С�ֱ�Ϊ��" << i1 << "-"<<size1<<" " << i2 << "-" << size2<< endl;
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