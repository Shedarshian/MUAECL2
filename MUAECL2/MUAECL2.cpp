// MUAECL2.cpp : �������̨Ӧ�ó������ڵ㡣
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
		//������
		ifstream in(argv[1]);
		//ifstream in("D:\\A.ѧϰ��Learning��\\A2.Mathematics\\VC\\C++workspace\\MUAECL2\\in.txt");
		Tokenizer tokenizer(in);
		Token* t = nullptr;
		do {
			if (t != nullptr)
				delete t;
			t = tokenizer.popToken();
			cout << t->debug_out() << " " << tokenizer.debug_lineNo() << endl;
		} while (t->type() != Op::TokenType::End);
	}
	catch (ExceptionWithLineNo &e) {
		cerr << e.lineNo << " " << e.what() << endl;
	}
	Parser::clear();
    return 0;
}

