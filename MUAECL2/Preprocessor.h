#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
using namespace std;

class Preprocessor {
public:
	static void search(string filename, filesystem::path currentpath, const vector<filesystem::path>& extrasearchpath, int lineNo, ostream& out);
	static pair<vector<string>, vector<string>> process(istream &in, ostream &out, filesystem::path originalpath, const vector<filesystem::path>& extrasearchpath);
};

