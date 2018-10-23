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
	//while(1)����հ���ע��
	while (1) {
		//�������հף������
		if (nextChar == ' ' || nextChar == '\t' || addLine(nextChar)) {
			nextChar = ReadStream.get();
			continue;
		}
		//���ĵ��������򷵻ؽ���token
		if (nextChar == EOF)
			return new Token_End(lineNo);
		//���������+misc char����
		if (Op::Ch::OperatorChar.find(nextChar) != Op::Ch::OperatorChar.end()) {
			//�������
			string op = "";
			//û����ϵ����������������ţ����߽����е�һЩ�����
			if (nextChar == '.' || nextChar == '(' || nextChar == ')' || nextChar == '[' || nextChar == ']' || nextChar == '{' || nextChar == '}' || nextChar == ',' || nextChar == ';' || nextChar == ':') {
				willBeNegative = !(nextChar == ')' || nextChar == ']');
				op += nextChar;
				nextChar = ReadStream.get();
				return new Token_Operator(lineNo, Op::Ch::ToOperator(op));
			}
			//���Ÿ��ţ������÷��˺ţ�ȡ��ַ����λ�룬���κμ���ǰ׺һԪ�����Ҳ�Ƕ�Ԫ������������
			if ((nextChar == '-' || nextChar == '*' || nextChar == '&') && willBeNegative) {
				op += nextChar;
				nextChar = ReadStream.get();
				willBeNegative = true; //�ƺ�û�ã���ʾһ��û�䣬����--Aȡ��θ���
				return new Token_Operator(lineNo, Op::Ch::ToOperator("(" + op + ")"));
			}
			//&��|����Ϊ��&&=��||=�����鷳һЩ
			if (nextChar == '&' || nextChar == '|') {
				willBeNegative = true;
				op += nextChar;
				nextChar = ReadStream.get();
				//op���ȿ϶�Ϊ1
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
			//����
			if (nextChar == '/') {
				op += nextChar;
				nextChar = ReadStream.get();
				//���Դ�"//"��ʼ��"\n"Ϊֹ���ı�
				if (nextChar == '/') {
					while (nextChar != '\n' && nextChar != EOF)
						nextChar = ReadStream.get();
					continue;
				}
				//���Դ�"/*"��ʼ��"*/"Ϊֹ���ı�
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
		//�ַ���
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
		//��ʶ��
		if (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
			string s;
			while (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_' || nextChar >= '0' && nextChar <= '9') {
				s += nextChar;
				nextChar = ReadStream.get();
			}
			//keyword����Ϊ����
			willBeNegative = true;
			//����Ǽ���Ԥ�����
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
			//�����build-in type
			if (auto it = Op::Ch::StringToType.find(s); it != Op::Ch::StringToType.end())
				return new Token_KeywordType(lineNo, it->second);
			//�����keyword
			if (auto it = Op::Ch::StringToOperator.find(s); it != Op::Ch::StringToOperator.end())
				return new Token_Operator(lineNo, it->second);
			//�����const int
			if (auto it = ReadIns::constint.find(s); it != ReadIns::constint.end())
				return new Token_Literal(lineNo, it->second);
			//�����const float
			if (auto it = ReadIns::constfloat.find(s); it != ReadIns::constfloat.end())
				return new Token_Literal(lineNo, it->second);
			willBeNegative = false;
			return new Token_Identifier(lineNo, s);
		}
		//��ֵ
		if (nextChar >= '0' && nextChar <= '9') {
			willBeNegative = false;
			string s;
			if (nextChar == '0') {
				nextChar = ReadStream.get();
				if (nextChar == 'x') {
					//0x��ͷ����
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
