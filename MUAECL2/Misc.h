#pragma once
#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <exception>
using namespace std;

namespace Op {
	template<typename T1, typename T2>
	pair<T2, T1> swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	map<T2, T1> swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	enum Operator { Plus, Minus, Times, Divides, Mod, EqualTo, NotEqual, Less, LessEqual, Greater, GreaterEqual, Not, LogicalOr, LogicalAnd, BitOr, BitAnd, BitXor, Negative, Dot, And, Or };//sin cos tan...
	enum AssignmentOperator { Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual };
	enum Keyword { Int, Float, Point, If, Else, Elsif, For, While, Break, Continue, Goto };
	const unordered_set<char> OperatorChar = { '+','-','*','/','%','&','|','!','^','=','>','<','.' };

	const map<Operator, string> OperatorToString = { { Plus, "+" },{ Minus, "-" },{ Times, "*" },{ Divides, "/" },{ Mod, "%" },{ EqualTo, "==" },{ NotEqual, "!=" },{ Less, "<" },{ LessEqual, "<=" },{ Greater, ">" },{ GreaterEqual, ">=" },{ Not, "!" },{ LogicalOr, "||" },{ LogicalAnd, "&&" },{ BitOr, "|" },{ BitAnd, "&" },{ BitXor, "^" },{ Negative, "(-)" },{ Dot, "." },{ And, "and" },{ Or, "or" } };
	const map<string, Operator> StringToOperator = swap_map(OperatorToString);
	const map<AssignmentOperator, string> AssignmentOperatorToString = { { Equal,"=" }, { PlusEqual,"+=" },{ MinusEqual,"-=" },{ TimesEqual,"*=" },{ DividesEqual,"/=" },{ ModEqual,"%=" },{ LogicalOrEqual,"||=" },{ LogicalAndEqual,"&&=" },{ BitOrEqual,"|=" },{ BitAndEqual,"&=" },{ BitXorEqual,"^=" } };
	const map<string, AssignmentOperator> StringToAssignmentOperator = swap_map(AssignmentOperatorToString);
	const map<Keyword, string> KeywordToString = { { Int, "int" },{ Float,"float" },{ Point, "point" },{ If, "if" },{ Else,"else" },{ Elsif,"elsif" },{ For,"for" },{ While,"while" },{ Break,"break" },{ Continue,"continue" },{ Goto,"goto" } };
	const map<string, Keyword> StringToKeyword = swap_map(KeywordToString);

	string ToString(Operator op) { return OperatorToString.find(op)->second; }
	Operator ToOperator(string s) { return StringToOperator.find(s)->second; }
	string ToString(AssignmentOperator op) { return AssignmentOperatorToString.find(op)->second; }
	AssignmentOperator ToAssignmentOperator(string s) { return StringToAssignmentOperator.find(s)->second; }
	string ToString(Keyword wd) { return KeywordToString.find(wd)->second; }
	Keyword ToKeyword(string wd) { return StringToKeyword.find(wd)->second; }
}

using Operator = Op::Operator;
using Assignment = Op::AssignmentOperator;

class Token {
public:
	Token() {};
	~Token() {};
	enum Type { Token_Int, Token_Float, Token_String, Token_Identifier, Token_Operator, Token_Assignment, Token_Semicolon, Token_Colon, Token_Bra, Token_Ket, Token_BigBra, Token_BigKet, Token_End, Token_KeywordType, Token_KeywordIf, Token_KeywordElse, Token_KeywordElsif, Token_KeywordFor, Token_KeywordWhile, Token_KeywordBreak, Token_KeywordContinue, Token_KeywordGoto };
	static const map<Op::Keyword, Type> KeywordToType;
	static const map<char, Type> ControlChar;
	//类型
	virtual Type type() = 0;
	//debug输出
	virtual string debug_out() = 0;
	friend ostream& operator<< (ostream& stream, Token& token);
};

const map<Op::Keyword, Token::Type> Token::KeywordToType = { { Op::Int, Token_KeywordType },{ Op::Float,Token_KeywordType },{ Op::Point,Token_KeywordType },{ Op::If,Token_KeywordIf },{ Op::Else,Token_KeywordElse },{ Op::Elsif,Token_KeywordElsif },{ Op::For,Token_KeywordFor },{ Op::While,Token_KeywordWhile },{ Op::Break,Token_KeywordBreak },{ Op::Continue,Token_KeywordContinue } ,{ Op::Goto,Token_KeywordGoto } };
const map<char, Token::Type> Token::ControlChar = { { ';', Token_Semicolon }, { ':', Token_Colon }, { '(', Token_Bra }, { ')', Token_Ket }, { '{', Token_BigBra }, { '}', Token_BigKet } };

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}

//整型常量
class Token_Int :public Token {
public:
	Token_Int(int val) :val(val) {};
	Type type() override { return Type::Token_Int; };
	string debug_out() override { return to_string(val); };
private:
	int val;
};

//浮点型常量
class Token_Float :public Token {
public:
	Token_Float(float val) :val(val) {};
	Type type() override { return Type::Token_Float; };
	string debug_out() override { return to_string(val); };
private:
	float val;
};

//字符串常量
class Token_String :public Token {
public:
	Token_String(string val) :val(val) {};
	Type type() override { return Type::Token_String; };
	string debug_out() override { return "\"" + val + "\""; };
private:
	string val;
};

//标识符，包括变量函数线程以及ins
class Token_Identifier :public Token {
public:
	Token_Identifier(string val) :val(val) {};
	Type type() override { return Type::Token_Identifier; };
	string debug_out() override { return val; };
private:
	string val;
};

//关键字
class Token_Keyword :public Token {
public:
	Token_Keyword(Op::Keyword keyword) :val(keyword) {};
	Type type() override { return KeywordToType.find(val)->second; };
	string debug_out() override { return Op::ToString(val); };
private:
	Op::Keyword val;
};

//运算符
class Token_Operator :public Token {
public:
	Token_Operator(Operator val) :val(val) {};
	Type type() override { return Type::Token_Operator; };
	string debug_out() override { return Op::ToString(val); };
private:
	Operator val;
};

//赋值运算符
class Token_Assignment :public Token {
public:
	Token_Assignment(Assignment val) :val(val) {};
	Type type() override { return Type::Token_Assignment; };
	string debug_out() override { return Op::ToString(val); };
private:
	Assignment val;
};

//分号冒号大小括号
class Token_ControlChar :public Token {
public:
	Token_ControlChar(char c) :t(Token::ControlChar.find(c)->second) {};
	Token_ControlChar(Token::Type t) :t(t) {};
	Type type() override { return t; };
	string debug_out() override { return ";"; };
private:
	Token::Type t;
};

//文档结束
class Token_End :public Token {
public:
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