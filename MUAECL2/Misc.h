#pragma once
#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <exception>
using namespace std;

class Op {
public:
	template<typename T1, typename T2>
	static pair<T2, T1> swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	static map<T2, T1> swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	enum Operator { Plus, Minus, Times, Divides, Mod, EqualTo, NotEqual, Less, LessEqual, Greater, GreaterEqual, Not, LogicalOr, LogicalAnd, BitOr, BitAnd, BitXor, Negative, Dot, And, Or };//sin cos tan...
	enum AssignmentOperator { Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual };
	enum Keyword { Int, Float, Point, If, Else, Elsif, For, While, Break, Continue, Goto, Sub };
	static const unordered_set<char> OperatorChar;

	static const map<Operator, string> OperatorToString;
	static const map<string, Operator> StringToOperator;
	static const map<AssignmentOperator, string> AssignmentOperatorToString;
	static const map<string, AssignmentOperator> StringToAssignmentOperator;
	static const map<Keyword, string> KeywordToString;
	static const map<string, Keyword> StringToKeyword;

	static string ToString(Operator op) { return OperatorToString.find(op)->second; }
	static Operator ToOperator(string s) { return StringToOperator.find(s)->second; }
	static string ToString(AssignmentOperator op) { return AssignmentOperatorToString.find(op)->second; }
	static AssignmentOperator ToAssignmentOperator(string s) { return StringToAssignmentOperator.find(s)->second; }
	static string ToString(Keyword wd) { return KeywordToString.find(wd)->second; }
	static Keyword ToKeyword(string wd) { return StringToKeyword.find(wd)->second; }
};

using Operator = Op::Operator;
using Assignment = Op::AssignmentOperator;
using Keyword = Op::Keyword;

class Token {
public:
	Token(int lineNo) :lineNo(lineNo) {};
	~Token() {};
	enum Type { Token_Int, Token_Float, Token_String, Token_Identifier, Token_Operator, Token_Equal, Token_Assignment, Token_Semicolon, Token_Colon, Token_Bra, Token_Ket, Token_BigBra, Token_BigKet, Token_Comma, Token_End, Token_KeywordType, Token_KeywordIf, Token_KeywordElse, Token_KeywordElsif, Token_KeywordFor, Token_KeywordWhile, Token_KeywordBreak, Token_KeywordContinue, Token_KeywordGoto, Token_KeywordSub };
	static const map<Op::Keyword, Type> KeywordToType;
	static const map<char, Type> ControlChar;
	static const map<Type, char> ControlToChar;
	//类型
	virtual Type type() = 0;
	//debug输出
	virtual string debug_out() { return to_string(lineNo); };
	friend ostream& operator<< (ostream& stream, Token& token);
private:
	const int lineNo;
};

//整型常量
class Token_Int :public Token {
public:
	Token_Int(int lineNo, int val) :Token(lineNo), val(val) {};
	Type type() override { return Type::Token_Int; };
	string debug_out() override { return to_string(val); };
private:
	int val;
};

//浮点型常量
class Token_Float :public Token {
public:
	Token_Float(int lineNo, float val) :Token(lineNo), val(val) {};
	Type type() override { return Type::Token_Float; };
	string debug_out() override { return to_string(val); };
private:
	float val;
};

//字符串常量
class Token_String :public Token {
public:
	Token_String(int lineNo, string val) :Token(lineNo), val(val) {};
	Type type() override { return Type::Token_String; };
	string debug_out() override { return "\"" + val + "\""; };
private:
	string val;
};

//标识符，包括变量函数线程以及ins
class Token_Identifier :public Token {
public:
	Token_Identifier(int lineNo, string val) :Token(lineNo), val(val) {};
	Type type() override { return Type::Token_Identifier; };
	string debug_out() override { return val; };
private:
	string val;
};

//关键字
class Token_Keyword :public Token {
public:
	Token_Keyword(int lineNo, Op::Keyword keyword) :Token(lineNo), val(keyword) {};
	Type type() override { return KeywordToType.find(val)->second; };
	string debug_out() override { return Op::ToString(val); };
private:
	Op::Keyword val;
};

//运算符
class Token_Operator :public Token {
public:
	Token_Operator(int lineNo, Operator val) :Token(lineNo), val(val) {};
	Type type() override { return Type::Token_Operator; };
	string debug_out() override { return Op::ToString(val); };
private:
	Operator val;
};

//赋值运算符
class Token_Assignment :public Token {
public:
	Token_Assignment(int lineNo, Assignment val) :Token(lineNo), val(val) {};
	Type type() override { return val == Assignment::Equal ? Type::Token_Equal : Type::Token_Assignment; };
	string debug_out() override { return Op::ToString(val); };
private:
	Assignment val;
};

//逗号分号冒号大小括号
class Token_ControlChar :public Token {
public:
	Token_ControlChar(int lineNo, char c) :Token(lineNo), t(Token::ControlChar.find(c)->second) {};
	Token_ControlChar(int lineNo, Token::Type t) :Token(lineNo), t(t) {};
	Type type() override { return t; };
	string debug_out() override { return string(1, Token::ControlToChar.find(t)->second); };
private:
	Token::Type t;
};

//文档结束
class Token_End :public Token {
public:
	Token_End(int lineNo) :Token(lineNo) {};
	Type type() override { return Type::Token_End; };
	string debug_out() override { return ""; };
};

//error-type
class ExceptionWithLineNo :public exception {
public:
	ExceptionWithLineNo(int lineNo) :lineNo(lineNo) {};
	const int lineNo;
};

class ErrOpenedString :public ExceptionWithLineNo {
public:
	ErrOpenedString(int lineNo) :ExceptionWithLineNo(lineNo) {};
	virtual const char* what() const throw() { return "Opened string."; }
};

class ErrDoubleDot :public ExceptionWithLineNo {
public:
	ErrDoubleDot(int lineNo) :ExceptionWithLineNo(lineNo) {};
	virtual const char* what() const throw() { return "Double Dot in a float number."; };
};

class ErrUnknownCharacter :public ExceptionWithLineNo {
public:
	ErrUnknownCharacter(int lineNo) :ExceptionWithLineNo(lineNo) {};
	virtual const char* what() const throw() { return "Unknown Character."; };
};

class ErrUnknownOperator :public ExceptionWithLineNo {
public:
	ErrUnknownOperator(int lineNo) :ExceptionWithLineNo(lineNo) {};
	virtual const char* what() const throw() { return "Unknown operator."; }
};