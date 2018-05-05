// MUAECL2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Tokenizer.h"
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
	//调试中
	ifstream in(argv[1]);
	Tokenizer tokenizer(in);
	Token* t = nullptr;
	do {
		tokenizer >> t;
		cout << t->debug_out() << " " << tokenizer.debug_lineNo() << endl;
	} while (t->type() != Token::Token_End);

    return 0;
}

