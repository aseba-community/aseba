#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <ctime>
#include <algorithm>

using namespace std;

// sorting functions

// comb sort ( from Wikipedia )
void aseba_comb_sort(int16_t* input, uint16_t size)
{
	uint16_t gap = size;
	uint16_t swapped = 0;
	uint16_t i;

	while ((gap > 1) || swapped)
	{
		if (gap > 1)
		{
#ifdef __C30__
			gap = __builtin_divud(__builtin_muluu(gap,4),5);
#else
			gap = (uint16_t)(((uint32_t)gap * 4) / 5);
#endif
		}

		swapped = 0;

		for (i = 0; gap + i < size; i++)
		{
			if (input[i] - input[i + gap] > 0)
			{
				int16_t swap = input[i];
				input[i] = input[i + gap];
				input[i + gap] = swap;
				swapped = 1;
			}
		}
	}
}

void aseba_heap_sort(int16_t* input, uint16_t size)
{
	int16_t swap;
	uint16_t i;
	// build heap one element by one element O(n log(n))
	for (i = 1; i < size; i++)
	{
		// We ensure the heap property by considering the new element indexed by current.
		// As long as its parent has a lower value than itself, we invert them and
		// update current.
		uint16_t current = i;
		while (current > 0)
		{
			// get parent
			uint16_t parent = (current-1) / 2;
			// if heap property is fulfilled, we break
			if (input[parent] >= input[current])
				break;
			// swap cur with parent
			swap = input[current];
			input[current] = input[parent];
			input[parent] = swap;
			// update current
			current = parent;
		}
	}
	// sort by moving the largest element to the partially-sorted array,
	// and then repair the heap propery
	for (i = size-1; i > 0; i--)
	{
		// We repair the heap property by inverting an element with its largest
		// child as long as one fo the child is larger.
		uint16_t current = 0;
		// first, swap front with new sorted position
		swap = input[i];
		input[i] = input[0];
		input[0] = swap;
		// then, repair the heap property
		while (2*current + 1 < i)
		{
			// find the largest child
			int16_t largest = current;
			int16_t left_child = 2*current + 1;
			int16_t right_child = left_child + 1;
			if (input[largest] < input[left_child])
				largest = left_child;
			if (right_child < i && input[largest] < input[right_child])
				largest = right_child;
			// is the heap property broken?
			if (largest != current)
			{
				// yes, swap the current with the largest child and continue
				swap = input[current];
				input[current] = input[largest];
				input[largest] = swap;
				current = largest;
			}
			else
				// no, stop processing this element
				break;
		}
	}
}

void aseba_std_sort(int16_t* input, uint16_t size)
{
	std::sort(&input[0], &input[size]);
}

// data generation methods

int16_t getRandom() { return (rand() % 65535) - 32768; }

int16_t getZero() { return 0; }

struct IncNumbers
{
	int16_t i { 0 };
	int16_t operator()() { return i++; }
};

struct DecNumbers
{
	int16_t i { 32767 };
	int16_t operator()() { return i--; }
};

// support data structure

struct TestCase
{
	string name;
	function<int()> func;
};

struct TestFunction
{
	string name;
	function<void(int16_t*, uint16_t)> func;
};

// main test loops

int main()
{
	srand(time(0));
	const vector<TestCase> test_cases = {
		{ "random data", getRandom },
		{ "constant 0", getZero },
		{ "incrementing numbers", IncNumbers() },
		{ "decrementing numbers", DecNumbers() }
	};
	const vector<TestFunction> test_functions = {
		{ "std::sort", aseba_std_sort },
		{ "comb sort", aseba_comb_sort },
		{ "heap sort", aseba_heap_sort }
	};
	
	for (auto& test_case : test_cases) {
		cout << "Testing case " << test_case.name << endl;
		int cur_size = 1;
		for (int i = 0; i < 16; i++) {
			// first generate data
			vector<vector<int16_t>> test_data;
			for (int j = 0; j < 1000; j++) {
				vector<int16_t> data(cur_size, 0);
				generate(data.begin(), data.end(), test_case.func);
				test_data.push_back(data);
			}
			// test different functions
			vector<vector<vector<int16_t>>> sorted_data;
			map<string, double> total_times;
			for (const auto& test_function: test_functions) {
				vector<vector<int16_t>> temp_data = test_data;
				auto start_time = chrono::system_clock::now();
				for (auto& test_array : temp_data) {
					test_function.func(&test_array[0], test_array.size());
				}
				auto end_time = chrono::system_clock::now();
				total_times[test_function.name] =
					chrono::duration<double>(end_time - start_time).count();
				sorted_data.push_back(temp_data);
			}
			// sanity check
			for (size_t method0 = 0; method0 < sorted_data.size(); ++method0) {
				for (size_t method1 = 0; method1 < method0; ++method1) {
					if (sorted_data[method0] != sorted_data[method1]) {
						cerr << "Sort functions " << test_functions[method0].name <<
							" and " << test_functions[method1].name <<
							" result mismatch for size " << i << endl;
						exit(1);
					}
				}
			}
			// show results
			cout << "job size: " << cur_size << "\n";
			for (const auto& total_time_KV: total_times) {
				cout << "1000 " << total_time_KV.first << " jobs elapsed time: " << total_time_KV.second << "s\n";
			}
				cur_size *= 2;
		}
		cout << endl;
	}
}

