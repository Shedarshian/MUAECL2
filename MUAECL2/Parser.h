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
	static const map<int, map<Op::TokenType, int>*> makeAction();
	static void clear();
	GrammarTree* analyse();
	void TypeCheck();
private:
	static GrammarTree* mergeTree(int id, stack<GrammarTree*>& s);
	//int表示移动，1~999为入栈，1001~1999为规约，0为accept，负数为error
	static const map<int, map<Op::TokenType, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	inline static set<int> ptr;
	static int action(int s, Op::TokenType t);
	static int gotostat(int s, Op::NonTerm t);
	Tokenizer &tokenizer;
	stack<GrammarTree*> s;
	GrammarTree* saveTree;
};

