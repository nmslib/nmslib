#pragma once
#include <unordered_map>
#include <queue>
using namespace std;
class FrequentItems
{
private:
	int * _values;
	int _k;
	int _size = 0;

	std::unordered_map<int, int> _keyToValLocMapping;
	std::unordered_map<int, int> _valLocToKeyMapping;

	std::queue<int> _emptyLocations;

public:
	FrequentItems(int k);
	~FrequentItems();
	void increment(int item);
	unsigned int* getTopk();
	void getTopk(unsigned int * outputs);
};