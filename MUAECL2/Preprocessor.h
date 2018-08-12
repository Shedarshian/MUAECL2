#pragma once
#include <iostream>
#include <vector>
using namespace std;

class Preprocessor {
public:
	static void process(istream &in, ostream &out, vector<string>& ecli, vector<string>& anim);
};

