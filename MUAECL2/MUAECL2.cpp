// MUAECL2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Tokenizer.h"
#include "Parser.h"
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
	Parser::initialize();
	try {
		//调试中
		ifstream in(argv[1]);
		//ifstream in("D:\\A.学习【Learning】\\A2.Mathematics\\VC\\C++workspace\\MUAECL2\\in.txt");
		Tokenizer tokenizer(in);
		Parser parser(tokenizer, true);
		tRoot* tree = parser.analyse();

	}
	catch (ExceptionWithLineNo &e) {
		cerr << e.lineNo << " " << e.what() << endl;
	}
	Parser::clear();
    return 0;
}

