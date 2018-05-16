#pragma once
#include <memory>
#include <fstream>
#include "Tokenizer.h"

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	//非终结符
	enum NonTerminator { stmt, stmts, subs, subv, subva, vdecl, insv, ini, inif, inia, exprf, expr, types };
private:
	Tokenizer &tokenizer;
	//int表示移动，1~999为入栈，1001~1999为规约，0为accept，负数为error
	static map<int, map<Op::Token, int>*> Action;
	static const map<NonTerminator, map<int, int>> Goto;
	static void initialize(ifstream& f);
	int action(int s, Op::Token t);
	int gotostat(int s, NonTerminator t);
};

