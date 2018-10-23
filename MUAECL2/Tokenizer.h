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
#include <utility>
#include "Misc.h"

using namespace std;

class Tokenizer {
public:
	Tokenizer(istream &ReadStream, const string& filename);		//���������ж�ȡ�ı�
	~Tokenizer();
	friend Tokenizer& operator>> (Tokenizer& t, Token*& token);		//������ʹ��
	Token* popToken();
	Token* peekToken();
	int debug_lineNo() { return lineNo; }
	inline auto&& popLineNoToPos() { return move(lineNoToPos); }
private:
	map<int, int> lineNoToPos;
	Token* getToken();
	Token* bufferToken;
	char nextChar;
	//�ڴ˲����������븺�ţ������� ������� �Ҵ����� ð�� �ֺ� ����� ��ֵ���"-"Ϊ���ţ�����Ϊ����
	bool willBeNegative;
	int lineNo;
	istream& ReadStream;
	const string& filename;
	inline bool addLine(char nextChar) {
		if (nextChar == '\n') {
			++lineNo;
			lineNoToPos[lineNo] = ReadStream.tellg();
			return true;
		}
		return false;
	};
};

