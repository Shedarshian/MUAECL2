#pragma once
#include <memory>
#include <fstream>
#include <stack>
#include <set>
#include "Tokenizer.h"
#include "GrammarTree.h"

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	static const map<int, map<Op::Token, int>*> makeAction();
	static void clear();
	GrammarTree* analyse();
private:
	Tokenizer &tokenizer;
	//int��ʾ�ƶ���1~999Ϊ��ջ��1001~1999Ϊ��Լ��0Ϊaccept������Ϊerror
	static const map<int, map<Op::Token, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	static set<int> ptr;
	stack<GrammarTree*> s;
	int action(int s, Op::Token t);
	int gotostat(int s, Op::NonTerm t);
};

