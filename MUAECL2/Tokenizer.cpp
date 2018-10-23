#include "stdafx.h"
#include <ctime>
#include "Tokenizer.h"

using namespace std;

Tokenizer::Tokenizer(istream& ReadStream, const string& filename) :ReadStream(ReadStream), lineNo(1), filename(filename) {
	nextChar = ReadStream.get();
	bufferToken = getToken();
}

Tokenizer::~Tokenizer() {
	delete bufferToken;
}

[[deprecated]] Tokenizer& operator>> (Tokenizer& t, Token*& token) {
	token = t.popToken();
	return t;
}

Token* Tokenizer::popToken() {
	Token* old = bufferToken;
	bufferToken = getToken();
	return old;
}

Token* Tokenizer::peekToken() {
	return bufferToken;
}

Token* Tokenizer::getToken(){
	//while(1)方便空白与注释
	while (1) {
		//若读到空白，则继续
		if (nextChar == ' ' || nextChar == '\t' || addLine(nextChar)) {
			nextChar = ReadStream.get();
			continue;
		}
		//若文档结束，则返回结束token
		if (nextChar == EOF)
			return new Token_End(lineNo);
		//若在运算符+misc char集中
		if (Op::Ch::OperatorChar.find(nextChar) != Op::Ch::OperatorChar.end()) {
			//是运算符
			string op = "";
			//没有组合的运算符，如点与括号，或者将来有的一些运算符
			if (nextChar == '.' || nextChar == '(' || nextChar == ')' || nextChar == '[' || nextChar == ']' || nextChar == '{' || nextChar == '}' || nextChar == ',' || nextChar == ';' || nextChar == ':') {
				willBeNegative = !(nextChar == ')' || nextChar == ']');
				op += nextChar;
				nextChar = ReadStream.get();
				return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
			}
			//减号负号，解引用符乘号，取地址符按位与，或任何既是前缀一元运算符也是二元运算符的运算符
			if ((nextChar == '-' || nextChar == '*' || nextChar == '&') && willBeNegative) {
				op += nextChar;
				nextChar = ReadStream.get();
				willBeNegative = true; //似乎没用，提示一下没变，允许--A取多次负号
				return new Token_Operator(lineNo, Op::Ch::ToOperator("(" + op + ")"));
			}
			//&和|，因为有&&=和||=所以麻烦一些
			if (nextChar == '&' || nextChar == '|') {
				willBeNegative = true;
				op += nextChar;
				nextChar = ReadStream.get();
				//op长度肯定为1
				if (nextChar == op[0]) {
					op += nextChar;
					nextChar = ReadStream.get();
				}
				if (nextChar == '=') {
					op += nextChar;
					nextChar = ReadStream.get();
					return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
				}
				return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
			}
			//除号
			if (nextChar == '/') {
				op += nextChar;
				nextChar = ReadStream.get();
				//忽略从"//"开始到"\n"为止的文本
				if (nextChar == '/') {
					while (nextChar != '\n' && nextChar != EOF)
						nextChar = ReadStream.get();
					continue;
				}
				//忽略从"/*"开始到"*/"为止的文本
				if (nextChar == '*') {
					bool isStar = false;
					nextChar = ReadStream.get();
					while (!(isStar && nextChar == '/') && nextChar != EOF) {
						isStar = (nextChar == '*');
						addLine(nextChar);
						nextChar = ReadStream.get();
					}
					if (nextChar != EOF) nextChar = ReadStream.get();
					continue;
				}
				if (nextChar == '=') {
					op += nextChar;
					nextChar = ReadStream.get();
					return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
				}
				return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
			}
			//+-*/%^<>!=
			willBeNegative = true;
			op += nextChar;
			nextChar = ReadStream.get();
			if (nextChar == '=') {
				op += nextChar;
				nextChar = ReadStream.get();
				return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
			}
			return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
		}
		//字符串
		if (nextChar == '"') {
			willBeNegative = false;
			string s;
			bool changed = false;
			nextChar = ReadStream.get();
			while (changed || nextChar != '"') {
				if (nextChar == '\n' || nextChar == EOF)
					throw(ErrOpenedString(lineNo));
				changed = (nextChar == '\\');
				s += nextChar;
				nextChar = ReadStream.get();
			}
			nextChar = ReadStream.get();
			return new Token_Literal(lineNo, s);
		}
		//标识符
		if (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
			string s;
			while (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_' || nextChar >= '0' && nextChar <= '9') {
				s += nextChar;
				nextChar = ReadStream.get();
			}
			//keyword后面为负号
			willBeNegative = true;
			//如果是几种预定义宏
			if (s == "__DATE__") {
				char c[9]; time_t t; tm time;
				std::time(&t);
				localtime_s(&time, &t);
				strftime(c, 9, "%F", &time);
				return new Token_Literal(lineNo, c);
			}
			if (s == "__TIME__") {
				char c[9]; time_t t; tm time;
				std::time(&t);
				localtime_s(&time, &t);
				strftime(c, 9, "%T", &time);
				return new Token_Literal(lineNo, c);
			}
			if (s == "__FILE__")
				return new Token_Literal(lineNo, filename.substr(filename.find_last_of("/") + 1));
			if (s == "__LINE__")
				return new Token_Literal(lineNo, lineNo);
			if (s == "pi")
				return new Token_Literal(lineNo, (float)3.14159265);
			//如果是build-in type
			if (auto it = Op::Ch::StringToType.find(s); it != Op::Ch::StringToType.end())
				return new Token_KeywordType(lineNo, it->second);
			//如果是keyword
			if (auto it = Op::Ch::StringToOperator.find(s); it != Op::Ch::StringToOperator.end())
				return new Token_Operator(lineNo, it->second);
			//如果是const int
			if (auto it = ReadIns::constint.find(s); it != ReadIns::constint.end())
				return new Token_Literal(lineNo, it->second);
			//如果是const float
			if (auto it = ReadIns::constfloat.find(s); it != ReadIns::constfloat.end())
				return new Token_Literal(lineNo, it->second);
			willBeNegative = false;
			return new Token_Identifier(lineNo, s);
		}
		//数值
		if (nextChar >= '0' && nextChar <= '9') {
			willBeNegative = false;
			string s;
			if (nextChar == '0') {
				nextChar = ReadStream.get();
				if (nextChar == 'x') {
					//0x开头数字
					while (1) {
						nextChar = ReadStream.get();
						if (nextChar >= '0' && nextChar <= '9' || nextChar >= 'A' && nextChar <= 'F' || nextChar >= 'a' && nextChar <= 'f')
							s += nextChar;
						else
							break;
					}
					return new Token_Literal(lineNo, stoi(s, 0, 16));
				}
				else
					s = '0';
			}
			bool dotted = false;
			while (1) {
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
					nextChar = ReadStream.get();
					break;
				}
				else
					break;
				nextChar = ReadStream.get();
			}
			if (!dotted)
				return new Token_Literal(lineNo, stoi(s));
			else
				return new Token_Literal(lineNo, stof(s));
		}
		throw(ErrUnknownCharacter(lineNo));
	}
}
