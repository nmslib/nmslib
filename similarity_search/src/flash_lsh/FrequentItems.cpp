#pragma once
#include "FrequentItems.h"
using namespace std;

/* Constructor. */
FrequentItems::FrequentItems(int k)
{
	_k = k;
	_values = new int[_k](); // An array holding _k values. 
	_size = 0;
	for (int i = 0; i < _k; i++)
	{
		_emptyLocations.push(i); // FIFO, first out will be the smallest number i = 0. 
	}
}

void FrequentItems::increment(int item)
{
	// If item is not found in _keyToValLocMapping. 
	if (_keyToValLocMapping.find(item) == _keyToValLocMapping.end())
	{

		// If the lossy counter is saturated. Loss the counts. 
		if (_size == _k)
		{ //clean 
			for (int i = 0; i < _k; i++)
			{
				_values[i]--;
				if (_values[i] == -1)
				{
					_emptyLocations.push(i);
					_size--;
					int key = _valLocToKeyMapping[i];
					_keyToValLocMapping.erase(key);
					_valLocToKeyMapping.erase(i);
				}
			}
		}

		// If still space in the counter. 
		if (_size < _k)
		{
			int loc = _emptyLocations.front();
			_emptyLocations.pop();
			_values[loc] = 1;
			_keyToValLocMapping[item] = loc;
			_valLocToKeyMapping[loc] = item;
			_size++;
		}
	}
	else { // If item found in the mapping. Increase the counts. 
		_values[_keyToValLocMapping[item]]++;
	}
}

unsigned int* FrequentItems::getTopk()
{
	unsigned int * heavyhitters = new unsigned int[_keyToValLocMapping.size() + 2];
	heavyhitters[0] = _keyToValLocMapping.size(); //1 reserved for id, 0 for size.
	int count = 2;
	for (unordered_map<int, int>::const_iterator it = _keyToValLocMapping.begin(); it != _keyToValLocMapping.end(); ++it) {
		int val = it->first;
		heavyhitters[count] = val;
		count++;
	}

	return heavyhitters;
}

void FrequentItems::getTopk(unsigned int * outputs)
{
	int count = 0;
	for (unordered_map<int, int>::const_iterator it = _keyToValLocMapping.begin(); it != _keyToValLocMapping.end(); ++it) {
		int val = it->first;
		outputs[count] = val;
		count++;
	}
}

FrequentItems::~FrequentItems()
{
	delete[] _values;
}