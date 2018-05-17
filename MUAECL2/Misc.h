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
	constexpr static pair<T2, T1> swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	constexpr static map<T2, T1> swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	enum Token { Identifier, Number, LogicalOr, LogicalAnd, Or, And, BitOr, BitXor, BitAnd, EqualTo, NotEqual, Greater, GreaterEqual, Less, LessEqual, Plus, Minus, Times, Divide, Mod, Negative, Not, Deref, Address, Dot, MidBra, MidKet, Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual, Sub, Type, If, Else, While, For, Goto, Break, Continue, Colon, Semicolon, Comma, Bra, Ket, BigBra, BigKet, End };
	enum BuiltInType { type_error, Void, Int, Float, Point };
	//非终结符
	enum NonTerm { stmt, stmts, subs, subv, subva, vdecl, insv, ini, inif, inia, exprf, expr, types };
	static const unordered_set<char> OperatorChar;

	static const map<Token, string> OperatorToString;
	static const map<string, Token> StringToOperator;
	static const map<BuiltInType, string> TypeToString;
	static const map<string, BuiltInType> StringToType;

	static string ToString(Token op) { return OperatorToString.find(op)->second; }
	static Token ToOperator(string s) { return StringToOperator.find(s)->second; }
	static string ToString(BuiltInType op) { return TypeToString.find(op)->second; }
	static BuiltInType ToType(string s) { return StringToType.find(s)->second; }
};

class Token {
public:
	Token(int lineNo) :lineNo(lineNo) {};
	virtual ~Token() {};
	//类型
	virtual Op::Token type() = 0;
	//检验
	virtual bool isIdNum() { return false; };
	virtual bool isExprFollow() { return false; };
	//取数
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
	Op::Token type() override { return Op::Token::Number; };
	bool isIdNum() override { return true; };
	string debug_out() override { return to_string(val); };
private:
	int val;
};

//浮点型常量
class Token_Float :public Token {
public:
	Token_Float(int lineNo, float val) :Token(lineNo), val(val) {};
	Op::Token type() override { return Op::Token::Number; };
	bool isIdNum() override { return true; };
	string debug_out() override { return to_string(val); };
private:
	float val;
};

//字符串常量
class Token_String :public Token {
public:
	Token_String(int lineNo, string val) :Token(lineNo), val(val) {};
	Op::Token type() override { return Op::Token::Number; };
	string debug_out() override { return "\"" + val + "\""; };
private:
	string val;
};

//标识符，包括变量函数线程以及ins
class Token_Identifier :public Token {
public:
	Token_Identifier(int lineNo, string val) :Token(lineNo), val(val) {};
	Op::Token type() override { return Op::Token::Identifier; };
	bool isIdNum() override { return true; };
	string debug_out() override { return val; };
private:
	string val;
};

//关键字,运算符,赋值运算符,逗号分号冒号大小括号
class Token_Operator :public Token {
public:
	Token_Operator(int lineNo, Op::Token val) :Token(lineNo), val(val) {};
	Op::Token type() override { return val; };
	bool isExprFollow() override { return val == Op::Token::Colon || val == Op::Token::BigKet || val == Op::Token::Comma || val == Op::Token::Ket || val == Op::Token::Semicolon; };
	string debug_out() override { return Op::ToString(val); };
private:
	Op::Token val;
};

//内建类型
class Token_KeywordType :public Token_Operator {
public:
	Token_KeywordType(int lineNo, Op::BuiltInType type) :Token_Operator(lineNo, Op::Token::Type), val(type) {};
	string debug_out() override { return Op::ToString(val); };
private:
	Op::BuiltInType val;
};

//文档结束
class Token_End :public Token {
public:
	Token_End(int lineNo) :Token(lineNo) {};
	Op::Token type() override { return Op::Token::End; };
	string debug_out() override { return ""; };
};

//error-type
class ExceptionWithLineNo :public exception {
public:
	ExceptionWithLineNo(int lineNo) :lineNo(lineNo) {};
	const int lineNo;
};

class ErrDesignApp :public exception {
public:
	ErrDesignApp(const char* what) :s("Design Error :"s + what + " Please Contact shedarshian@gmail.com"s) {};
	virtual const char* what() const throw() { return s.c_str(); }
	const string s;
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

class ErrIllegalToken :public ExceptionWithLineNo {
public:
	ErrIllegalToken(int lineNo, Token* token) :ExceptionWithLineNo(lineNo) {
		s = "illegal token "s + token->debug_out() + "."s;
	};
	virtual const char* what() const throw() { return s.c_str(); }
private:
	string s;
};