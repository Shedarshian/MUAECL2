#pragma once
#include <memory>
#include "Tokenizer.h"

using namespace std;

class Parser {
public:
	Parser(Tokenizer &tokenizer);
	~Parser();
	//���ս��
	enum NonTerminator { stmt, stmts, subs, subv, subva, vdecl, insv, ini, inif, inia, exprf, expr };
private:
	Tokenizer &tokenizer;
	//int��ʾ�ƶ���0~1000Ϊ��ջ��1001~1999Ϊ��Լ��-1Ϊaccept
	static const map<int, map<Op::Token, int>*> Action;
	static const map<NonTerminator, map<int, int>> Goto;
	int action(int s, Op::Token t);
	int gotostat(int s, NonTerminator t);
};

