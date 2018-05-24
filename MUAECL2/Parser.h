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
	//int��ʾ�ƶ���1~999Ϊ��ջ��1001~1999Ϊ��Լ��0Ϊaccept������Ϊerror
	static const map<int, map<Op::TokenType, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	inline static set<int> ptr;
	static int action(int s, Op::TokenType t);
	static int gotostat(int s, Op::NonTerm t);
	Tokenizer &tokenizer;
	stack<GrammarTree*> s;
	GrammarTree* saveTree;
};

