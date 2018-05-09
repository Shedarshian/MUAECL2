#pragma once
#include "Tokenizer.h"
#include <memory>

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	//非终结符
	enum NonTerminator { stmt = 2000, stmts, vdecl, vdecla, insv, insva, ifa, ini, inif, inia, exprf };
	enum Terminator { identifier = 3000, semicolon, colon, comma, bra, ket, bigbra, bigket, end, equal, expression, assignment_operator, type, if_bra, elsif_bra, else_bra, while_bra, for_bra, goto_id_colon };
private:
	Tokenizer &tokenizer;
	//int表示移动，0~1000为入栈，1001~1999为规约，-1为accept
	static const map<int, map<Terminator, int>*> Action;
	static const map<NonTerminator, map<int, int>> Goto;
	int action(int s, Terminator t);
	int gotostat(int s, NonTerminator t);
};

