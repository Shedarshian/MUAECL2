﻿#pragma once
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
	static void clear() noexcept;
	tRoot* analyse();
	void TypeCheck();
	fRoot Output(bool is_debug_compile, const vector<string>& ecli, const vector<string>& anim, rapidjson::Document& jsondoc_dbginfo, rapidjson::Value& jsonval_dbginfo_eclfile);
private:
	static GrammarTree* mergeTree(int id, stack<pair<int, GrammarTree*>>& s);
	//int表示移动，1~999为入栈，1001~1999为规约，0为accept，负数为error
	static map<int, map<Op::TokenType, int>*> Action;
	static const map<Op::NonTerm, map<int, int>> Goto;
	static set<int> ptr;
	static int action(int s, Op::TokenType t);
	static int gotostat(int s, Op::NonTerm t);
	Tokenizer &tokenizer;
	stack<pair<int, GrammarTree*>> s;
	tRoot* saveTree;
	static constexpr bool debug = false;
};

