#include "stdafx.h"
#include "Tokenizer.h"


Tokenizer::Tokenizer(istream& ReadStream) :ReadStream(ReadStream), lineNo(1) {
	ReadStream.get(nextChar);
}

Tokenizer::~Tokenizer() {}

Tokenizer& Tokenizer::operator>> (Token* token) {
	token = getToken();
	return (*this);
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
			return new Token_End();
		//���������char����
		if (Op::OperatorChar.find(nextChar) != Op::OperatorChar.end()) {
			//�������
			string op = "";
			//��ֵ��"="�ͱȽϵ�"=="
			if (nextChar == '=') {
				willBeNegative = true;
				ReadStream.get(nextChar);
				if (nextChar == '=') {
					ReadStream.get(nextChar);
					return new Token_Operator(Op::Operator::EqualTo);
				}
				else
					return new Token_Assignment(Op::AssignmentOperator::Equal);
			}
			//û����ϵ����������㣬���߽����е�һЩ�����
			if (nextChar == '.') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				return new Token_Operator(Op::ToOperator(op));
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
					return new Token_Assignment(Op::ToAssignmentOperator(op));
				}
				return new Token_Operator(Op::ToOperator(op));
			}
			//<>!������!= <= >=���Ǹ�ֵ��������Էֿ�����
			if (nextChar == '!' || nextChar == '<' || nextChar == '>') {
				willBeNegative = true;
				op += nextChar;
				ReadStream.get(nextChar);
				if (nextChar == '=') {
					op += nextChar;
					ReadStream.get(nextChar);
				}
				return new Token_Operator(Op::ToOperator(op));
			}
			//����
			if (nextChar == '/') {
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
			}
			//�����븺�ţ����κμ���ǰ׺һԪ�����Ҳ�Ƕ�Ԫ������������
			if (nextChar == '-' && willBeNegative) {
				ReadStream.get(nextChar);
				willBeNegative = true;//�ƺ�û�ã���ʾһ��û��
				return new Token_Operator(Op::ToOperator("(" + string(nextChar, 1) + ")"));
			}
			//+-*/%^
			willBeNegative = true;
			op += nextChar;
			ReadStream.get(nextChar);
			if (nextChar == '=') {
				op += nextChar;
				return new Token_Assignment(Op::ToAssignmentOperator(op));
			}
			return new Token_Operator(Op::ToOperator(op));
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
			return new Token_String(s);
		}
		//�ֺţ�ð�ţ�����
		if (Token::ControlChar.find(nextChar) != Token::ControlChar.end()) {
			char c = nextChar;
			ReadStream.get(nextChar);
			willBeNegative = (c != ')');
			return new Token_ControlChar(c);
		}
		//��ʶ��
		if (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
			willBeNegative = false;
			string s;
			while (nextChar >= 'a' && nextChar <= 'z' || nextChar >= 'A' && nextChar <= 'Z' || nextChar == '_') {
				s += nextChar;
				ReadStream.get(nextChar);
			}
			return new Token_Identifier(s);
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
						if (nextChar >= '0' && nextChar <= '9' || nextChar >= 'A' && nextChar <= 'F' || nextChar >= 'a' && nextChar <= 'f') {
							s += nextChar;
						}
						else
							break;
					}
					return new Token_Int(stoi(s, 0, 16));
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
				return new Token_Int(stoi(s));
			else
				return new Token_Float(stof(s));
		}
		throw(ErrUnknownCharacter(lineNo));
	}
}
