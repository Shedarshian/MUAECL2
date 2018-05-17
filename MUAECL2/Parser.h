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
	//int表示移动，1~999为入栈，1001~1999为规约，0为accept，负数为error
	static const map<int, map<Op::Token, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	static set<int> ptr;
	stack<GrammarTree*> s;
	int action(int s, Op::Token t);
	int gotostat(int s, Op::NonTerm t);
};

