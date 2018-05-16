#pragma once
#include <memory>
#include <fstream>
#include "Tokenizer.h"

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
private:
	Tokenizer &tokenizer;
	//int��ʾ�ƶ���1~999Ϊ��ջ��1001~1999Ϊ��Լ��0Ϊaccept������Ϊerror
	static map<int, map<Op::Token, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	static void initialize(ifstream& f);
	int action(int s, Op::Token t);
	int gotostat(int s, Op::NonTerm t);
};

