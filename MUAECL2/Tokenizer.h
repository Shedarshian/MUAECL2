/////////////////////////////////////////////////////////////////////////////////////
//***********************************************************************************
//*   ----  [ \t\n]       ------  [a-zA-Z_]
//*   |  |                |    |
//*   |  v   [a-zA-Z_]    |    v            else
//*   --(0) -----------> (1)[Identifier] ------------> (0)
//*      |                                back output
//*      |   +-*/%^              =
//*      ---------------> (2) --------> (0)
//*      |     <>!     (2')|  output
//*      |                 |
//*      |                 | else operator
//*      |                 -----------> Error
//*      |                 |
//*      |                 |   else
//*      |                 -----------> (0)
//*      |                  back output
//*      |     &/|                 =
//*      ---------------> (3/4) --------> (0)
//*      |                  |
//*      |                  |    &/|             =
//*      |                  ------------> (5) ---------> (0)
//*      |                                 |    else
//*      |                                 ------------> Error/(0)
//*      |      =                 =
//*      ---------------> (6) ----------> (0)
//*
//*
//*
//***********************************************************************************
/////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <iostream>
#include "Misc.h"

using namespace std;

class Tokenizer {
public:
	Tokenizer(istream &ReadStream);		//从输入流中读取文本
	~Tokenizer();
	friend Tokenizer& operator>> (Tokenizer& t, Token*& token);		//不建议使用
	Token* popToken();
	Token* peekToken();
	int debug_lineNo() { return lineNo; }
private:
	Token* getToken();
	Token* bufferToken;
	char nextChar;
	//在此步分析减号与负号，左括号 左大括号 右大括号 冒号 分号 运算符 赋值后的"-"为负号，此外为减号
	bool willBeNegative;
	int lineNo;
	istream& ReadStream;
};

