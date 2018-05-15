#pragma once
#include <memory>
#include "Tokenizer.h"

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	//非终结符
	enum NonTerminator { stmt, stmts, subs, subv, subva, vdecl, insv, ini, inif, inia, exprf, expr };
private:
	Tokenizer &tokenizer;
	//int表示移动，0~1000为入栈，1001~1999为规约，-1为accept
	static const map<int, map<Op::Token, int>*> Action;
	static const map<NonTerminator, map<int, int>> Goto;
	int action(int s, Op::Token t);
	int gotostat(int s, NonTerminator t);
};

