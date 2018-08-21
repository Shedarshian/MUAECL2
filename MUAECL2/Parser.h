#pragma once
#include <memory>
#include <fstream>
#include <stack>
#include <set>
#include <tuple>
#include "Tokenizer.h"
#include "GrammarTree.h"
#include "Misc2.h"

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	static void initialize();
	static void clear();
	tRoot* analyse();
	void TypeCheck();
	fRoot Output(const vector<string>& ecli, const vector<string>& anim);
private:
	static GrammarTree* mergeTree(int id, stack<pair<int, GrammarTree*>>& s);
	//int��ʾ�ƶ���1~999Ϊ��ջ��1001~1999Ϊ��Լ��0Ϊaccept������Ϊerror
	static map<int, map<Op::TokenType, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	static set<int> ptr;
	static int action(int s, Op::TokenType t);
	static int gotostat(int s, Op::NonTerm t);
	Tokenizer &tokenizer;
	stack<pair<int, GrammarTree*>> s;
	tRoot* saveTree;
	static constexpr bool debug = true;
};

