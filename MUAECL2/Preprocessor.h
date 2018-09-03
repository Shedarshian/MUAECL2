#pragma once
#include <iostream>
#include <vector>
#include <filesystem>
using namespace std;

class Preprocessor {
public:
	static pair<vector<string>, vector<string>> process(istream &in, ostream &out, const vector<filesystem::path>& searchpath);
};

