#include "stdafx.h"
#include "Tokenizer.h"

using namespace std;

Tokenizer::Tokenizer(istream& ReadStream) :ReadStream(ReadStream), lineNo(1) {
	ReadStream.get(nextChar);
}

Tokenizer::~Tokenizer() {}

Tokenizer& operator>> (Tokenizer& t, Token*& token) {
	token = t.getToken();
	return t;
}

Token* Tokenizer::getToken(){
	//while(1)方便空白与注释
	while (1) {
		//若读到空白，则继续
		if (nextChar == ' ' || nextChar == '\t' || (nextChar == '\n' && ++lineNo)) {
			ReadStream.get(nextChar);
			continue;
		}
		//若文档结束，则返回结束token
		if (nextChar == EOF)
			return new Token_End(lineNo);
		//若在运算符char集中
		if (Op::OperatorChar.find(nextChar) != Op::OperatorChar.end()) {
			//是运算符
			string op = "";
			//赋值的"="和比较的"=="
			if (nextChar == '=') {
				willBeNegative = true;
				ReadStream.get(nextChar);
				if (nextChar == '=') {
					ReadStream.get(nextChar);
					return new Token_Operator(lineNo, Op::Operator::EqualTo);
				}
				else
					return new Token_Assignment(lineNo, Op::AssignmentOperator::Equal);
			}
			//没有组合的运算符，如点，或者将来有的一些运算符
			if (nextChar == '.') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//&和|，因为有&&=和||=所以麻烦一些
			if (nextChar == '&' || nextChar == '|') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				//op长度肯定为1
				if (nextChar == op[0]) {
					op += nextChar;
					ReadStream.get(nextChar);
				}
				if (nextChar == '=') {
					op += nextChar;
					ReadStream.get(nextChar);
					return new Token_Assignment(lineNo, Op::ToAssignmentOperator(op));
				}
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//<>!，由于!= <= >=不是赋值运算符所以分开处理
			if (nextChar == '!' || nextChar == '<' || nextChar == '>') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				if (nextChar == '=') {
					op += nextChar;
					ReadStream.get(nextChar);
				}
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//除号
			if (nextChar == '/') {
				op += nextChar;
				ReadStream.get(nextChar);
				//忽略从"//"开始到"\n"为止的文本
				if (nextChar == '/') {
					while (nextChar != '\n' && nextChar != EOF)
						ReadStream.get(nextChar);
					continue;
				}
				//忽略从"/*"开始到"*/"为止的文本
				if (nextChar == '*') {
					bool isStar = false;
					ReadStream.get(nextChar);
					while (!(isStar && nextChar == '/') && nextChar != EOF) {
						isStar = (nextChar == '*');
						if (nextChar == '\n') lineNo++;
						ReadStream.get(nextChar);
					}
					if (nextChar != EOF) ReadStream.get(nextChar);
					continue;
				}
				if (nextChar == '=') {
					op += nextChar;
					ReadStream.get(nextChar);
					return new Token_Assignment(lineNo, Op::ToAssignmentOperator(op));
				}
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//减号与负号，或任何既是前缀一元运算符也是二元运算符的运算符
			if (nextChar == '-' && willBeNegative) {
				op += nextChar;
				ReadStream.get(nextChar);
				willBeNegative = true;//似乎没用，提示一下没变，允许--A取多次负号
				return new Token_Operator(lineNo, Op::ToOperator("(" + op + ")"));
			}
			//+-*/%^
			willBeNegative = true;
			op += nextChar;
			ReadStream.get(nextChar);
			if (nextChar == '=') {
				op += nextChar;
				ReadStream.get(nextChar);
				return new Token_Assignment(lineNo, Op::ToAssignmentOperator(op));
			}
			return new Token_Operator(lineNo, Op::ToOperator(op));
		}
		//字符串
		if (nextChar == '"') {
			willBeNegative = false;
			string s;
			bool changed = false;
			ReadStream.get(nextChar);
			while (changed || nextChar != '"') {
				if (nextChar == '\n' || nextChar == EOF)
					throw(ErrOpenedString(lineNo));
				changed = (nextChar == '\\');
				s += nextChar;
				ReadStream.get(nextChar);
			}
			ReadStream.get(nextChar);
			return new Token_String(lineNo, s);
		}
		//分号，冒号，括号
		if (Token::ControlChar.find(nextChar) != Token::ControlChar.end()) {
			char c = nextChar;
			ReadStream.get(nextChar);
			willBeNegative = (c != ')');
			return new Token_ControlChar(lineNo, c);
		}
		//标识符
		if (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
			willBeNegative = false;
			string s;
			while (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
				s += nextChar;
				ReadStream.get(nextChar);
			}
			//如果是keyword
			auto it = Op::StringToKeyword.find(s);
			if (it != Op::StringToKeyword.end())
				return new Token_Keyword(lineNo, it->second);
			//如果是and or sin cos sqrt
			auto it2 = Op::StringToOperator.find(s);
			if (it2 != Op::StringToOperator.end())
				return new Token_Operator(lineNo, it2->second);
			return new Token_Identifier(lineNo, s);
		}
		//数值
		if (nextChar >= '0' && nextChar <= '9') {
			willBeNegative = false;
			string s;
			if (nextChar == '0') {
				ReadStream.get(nextChar);
				if (nextChar == 'x') {
					//0x开头数字
					while (1) {
						ReadStream.get(nextChar);
						if (nextChar >= '0' && nextChar <= '9' || nextChar >= 'A' && nextChar <= 'F' || nextChar >= 'a' && nextChar <= 'f')
							s += nextChar;
						else
							break;
					}
					return new Token_Int(lineNo, stoi(s, 0, 16));
				}
				else
					s = '0';
			}
			s += nextChar;
			bool dotted = false;
			while (1) {
				ReadStream.get(nextChar);
				if (nextChar >= '0' && nextChar <= '9')
					s += nextChar;
				else if (nextChar == '.')
					if (dotted)
						throw(ErrDoubleDot(lineNo));
					else {
						dotted = true;
						s += nextChar;
					}
				else if (nextChar == 'f') {
					ReadStream.get(nextChar);
					break;
				}
				else
					break;
			}
			if (!dotted)
				return new Token_Int(lineNo, stoi(s));
			else
				return new Token_Float(lineNo, stof(s));
		}
		throw(ErrUnknownCharacter(lineNo));
	}
}
