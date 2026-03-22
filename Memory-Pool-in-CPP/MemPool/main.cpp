#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include "MemPool.h"


// Normal way of allocating in heap
class Object {
	int data;

public:

	Object(int data) : data(data) {}
	int get() const { 
		return data; 
	}
};


// Pooled way of allocating in heap
class Object1 {
	int data;

public:

	Object1(int data) : data(data) {}
	int get() const {
		return data;
	}

	void* operator new(const size_t size) {
		return memoryManager.Allocate();
	}

	void operator delete(void* memoryAddress) {
		return memoryManager.Deallocate(memoryAddress);
	}

	static MemoryManager memoryManager;
};

//Initializing static memory manager for Object1
MemoryManager Object1::memoryManager(sizeof(Object1), 64);

// Statistics
struct Stats {
	double mean_ms;
	double stddev_ms;
	double median_ms;
	double min_ms;
	double max_ms;
	double cv_pct; // claude helped me in making this struct
};

Stats compute_stats(std::vector<double>& samples) {
	std::sort(samples.begin(), samples.end());

	double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
	double mean = sum / samples.size();

	double sq_sum = 0.0;
	for (double s : samples) sq_sum += (s - mean) * (s - mean);
	double stddev = std::sqrt(sq_sum / (samples.size() - 1));

	double median = samples[samples.size() / 2];

	return {
		mean, stddev, median, samples.front(), samples.back(), (stddev / mean) * 100.0
	};
}

void print_stats(const std::string& label, const Stats& s) {
	std::cout << std::fixed << std::setprecision(4);
	std::cout << "\n[ " << label << " ]\n";
	std::cout << " Mean: " << std::setw(10) << s.mean_ms << " ms\n";
	std::cout << " Median: " << std::setw(10) << s.median_ms << " ms\n";
	std::cout << " StdDev: " << std::setw(10) << s.stddev_ms << " ms\n";
	std::cout << " Min: " << std::setw(10) << s.min_ms << " ms\n";
	std::cout << " Max: " << std::setw(10) << s.max_ms << " ms\n";
	std::cout << " CV: " << std::setw(10) << s.cv_pct << " % (lower  = more stable)\n";
}

// benchmak kernel
 
volatile int sink = 0; // prevents the compiler from optimizing away the object usage

template<typename T>
double run_once(int iterations) {
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < iterations; i++) {
		T* o1 = new T(i + 1);
		T* o2 = new T(i + 2);
		T* o3 = new T(i + 3);
		T* o4 = new T(i + 4);
		T* o5 = new T(i + 5);
		T* o6 = new T(i + 6);
		T* o7 = new T(i + 7);
		T* o8 = new T(i + 8);


		sink ^= o1->get() ^ o2->get() ^ o3->get() ^ o4->get() ^ o5->get() ^ o6->get() ^ o7->get() ^ o8->get();

		delete o1;
		delete o2;
		delete o3;
		delete o4;
		delete o5;
		delete o6;
		delete o7;
		delete o8;
	}

	auto end = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {

	constexpr int WARMUP_ROUNDS = 5;
	constexpr int MEASURE_ROUNDS = 30;
	constexpr int ITERATIONS = 50000;

	std::cout << "Warming up (" << WARMUP_ROUNDS << " rounds)...\n";
	for (int i = 0; i < WARMUP_ROUNDS; i++) {
		run_once<Object>(ITERATIONS);
		run_once<Object1>(ITERATIONS);
	}

	std::cout << "Measuring (" << MEASURE_ROUNDS << " rounds x " << ITERATIONS << " iterations)...\n";

	std::vector<double> normal_samples, pool_samples;
	normal_samples.reserve(MEASURE_ROUNDS);
	pool_samples.reserve(MEASURE_ROUNDS);

	for (int r = 0; r < MEASURE_ROUNDS; r++) {
		normal_samples.push_back(run_once<Object>(ITERATIONS));
		pool_samples.push_back(run_once<Object1>(ITERATIONS));
	}

	Stats ns = compute_stats(normal_samples);
	Stats ps = compute_stats(pool_samples);

	print_stats("Normal Allocation", ns);
	print_stats("Pool Allpcation", ps);

	double speedup = ns.mean_ms / ps.mean_ms;
	std::cout << "\n Speedup (mean): " << std::setprecision(2) << speedup << "x\n";
	std::cout << " (sink=" << sink << " -> ignore this, prevents dead-code elimination)\n";

	return 0;
}