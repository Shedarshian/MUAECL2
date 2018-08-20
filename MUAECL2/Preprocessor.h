#pragma once
#include <iostream>
#include <vector>
using namespace std;

class Preprocessor {
public:
	static pair<vector<string>, vector<string>> process(istream &in, ostream &out);
};

