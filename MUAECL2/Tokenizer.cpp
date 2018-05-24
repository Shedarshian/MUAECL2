#include "stdafx.h"
#include "Tokenizer.h"

using namespace std;

Tokenizer::Tokenizer(istream& ReadStream) :ReadStream(ReadStream), lineNo(1) {
	ReadStream.get(nextChar);
}

Tokenizer::~Tokenizer() {}

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
		if (nextChar == ' ' || nextChar == '\t' || (nextChar == '\n' && ++lineNo)) {
			ReadStream.get(nextChar);
			continue;
		}
		//���ĵ��������򷵻ؽ���token
		if (nextChar == EOF)
			return new Token_End(lineNo);
		//���������+misc char����
		if (Op::OperatorChar.find(nextChar) != Op::OperatorChar.end()) {
			//�������
			string op = "";
			//û����ϵ����������㣬���߽����е�һЩ�����
			if (nextChar == '.') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//���Ÿ��ţ������÷��˺ţ�ȡ��ַ����λ�룬���κμ���ǰ׺һԪ�����Ҳ�Ƕ�Ԫ������������
			if ((nextChar == '-' || nextChar == '*' || nextChar == '&') && willBeNegative) {
				op += nextChar;
				ReadStream.get(nextChar);
				willBeNegative = true; //�ƺ�û�ã���ʾһ��û�䣬����--Aȡ��θ���
				return new Token_Operator(lineNo, Op::ToOperator("(" + op + ")"));
			}
			//&��|����Ϊ��&&=��||=�����鷳һЩ
			if (nextChar == '&' || nextChar == '|') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				//op���ȿ϶�Ϊ1
				if (nextChar == op[0]) {
					op += nextChar;
					ReadStream.get(nextChar);
				}
				if (nextChar == '=') {
					op += nextChar;
					ReadStream.get(nextChar);
					return new Token_Operator(lineNo, Op::ToOperator(op));
				}
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//����
			if (nextChar == '/') {
				op += nextChar;
				ReadStream.get(nextChar);
				//���Դ�"//"��ʼ��"\n"Ϊֹ���ı�
				if (nextChar == '/') {
					while (nextChar != '\n' && nextChar != EOF)
						ReadStream.get(nextChar);
					continue;
				}
				//���Դ�"/*"��ʼ��"*/"Ϊֹ���ı�
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
					return new Token_Operator(lineNo, Op::ToOperator(op));
				}
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			//+-*/%^<>!=
			willBeNegative = (nextChar != ')');
			op += nextChar;
			ReadStream.get(nextChar);
			if (nextChar == '=') {
				op += nextChar;
				ReadStream.get(nextChar);
				return new Token_Operator(lineNo, Op::ToOperator(op));
			}
			return new Token_Operator(lineNo, Op::ToOperator(op));
		}
		//�ַ���
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
			return new Token_Literal(lineNo, s);
		}
		//��ʶ��
		if (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
			string s;
			while (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
				s += nextChar;
				ReadStream.get(nextChar);
			}
			//keyword����Ϊ����
			willBeNegative = true;
			//�����build-in type
			if (auto it = Op::StringToType.find(s); it == Op::StringToType.end())
				return new Token_KeywordType(lineNo, it->second);
			//�����keyword
			if (auto it = Op::StringToOperator.find(s); it != Op::StringToOperator.end())
				return new Token_Operator(lineNo, it->second);
			willBeNegative = false;
			return new Token_Identifier(lineNo, s);
		}
		//��ֵ
		if (nextChar >= '0' && nextChar <= '9') {
			willBeNegative = false;
			string s;
			if (nextChar == '0') {
				ReadStream.get(nextChar);
				if (nextChar == 'x') {
					//0x��ͷ����
					while (1) {
						ReadStream.get(nextChar);
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
				return new Token_Literal(lineNo, stoi(s));
			else
				return new Token_Literal(lineNo, stof(s));
		}
		throw(ErrUnknownCharacter(lineNo));
	}
}
