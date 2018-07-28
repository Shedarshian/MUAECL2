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
	Parser(Tokenizer &tokenizer, const bool debug);
	~Parser();
	static void initialize();
	static void clear();
	tRoot* analyse();
	void TypeCheck();
private:
	static GrammarTree* mergeTree(int id, stack<GrammarTree*>& s);
	//int��ʾ�ƶ���1~999Ϊ��ջ��1001~1999Ϊ��Լ��0Ϊaccept������Ϊerror
	static map<int, map<Op::TokenType, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	static set<int> ptr;
	static int action(int s, Op::TokenType t);
	static int gotostat(int s, Op::NonTerm t);
	Tokenizer &tokenizer;
	stack<GrammarTree*> s;
	tRoot* saveTree;
	const bool debug;
};

