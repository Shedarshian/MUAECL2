#pragma once
#include "Tokenizer.h"
#include <memory>

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	//���ս��
	enum NonTerminator { stmt = 2000, stmts, vdecl, vdecla, insv, insva, ifa, ini, inif, inia, exprf };
	enum Terminator { identifier = 3000, semicolon, colon, comma, bra, ket, bigbra, bigket, end, equal, expression, assignment_operator, type, if_bra, elsif_bra, else_bra, while_bra, for_bra, goto_id_colon };
private:
	Tokenizer &tokenizer;
	//int��ʾ�ƶ���0~1000Ϊ��ջ��1001~1999Ϊ��Լ��-1Ϊaccept
	static const map<int, map<Terminator, int>*> Action;
	static const map<NonTerminator, map<int, int>> Goto;
	int action(int s, Terminator t);
	int gotostat(int s, NonTerminator t);
};

