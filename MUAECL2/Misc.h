#pragma once
#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <memory>
#include <exception>
using namespace std;

struct mRealType;
class Token;
class Op {
public:
	template<typename T1, typename T2>
	constexpr static const pair<T2, T1>& swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	constexpr static const map<T2, T1>& swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	enum class TokenType { Identifier, Number, LogicalOr, LogicalAnd, Or, And, BitOr, BitXor, BitAnd, EqualTo, NotEqual, Greater, GreaterEqual, Less, LessEqual, Plus, Minus, Times, Divide, Mod, Negative, Not, Deref, Address, Dot, MidBra, MidKet, Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual, Sub, Type, If, Else, While, For, Goto, Break, Continue, Colon, Semicolon, Comma, Bra, Ket, BigBra, BigKet, End };
	enum class BuiltInType { type_error, Void, Int, Float, Point };
	//非终结符
	enum class NonTerm { stmt, stmts, subs, subv, vdecl, insv, ini, inif, inia, exprf, expr, types };
	static const unordered_set<char> OperatorChar;

	static const map<TokenType, string> OperatorToString;
	static const map<string, TokenType> StringToOperator;
	static const map<BuiltInType, string> TypeToString;
	static const map<string, BuiltInType> StringToType;

	inline static string ToString(TokenType op) { return OperatorToString.find(op)->second; }
	inline static TokenType ToOperator(string s) { return StringToOperator.find(s)->second; }
	inline static string ToString(BuiltInType op) { return TypeToString.find(op)->second; }
	inline static BuiltInType ToType(string s) { return StringToType.find(s)->second; }
	static NonTerm ToType(int id) {
		if (id >= 2 && id <= 11) return NonTerm::stmt;
		if (id >= 22 && id <= 28) return NonTerm::expr;
		if (id == 12) return NonTerm::inia;
		if (id == 14 || id == 15) return NonTerm::inif;
		if (id == 16 || id == 17) return NonTerm::ini;
		if (id == 20 || id == 21) return NonTerm::exprf;
		throw(ErrDesignApp("NonTerminator saved error."));
	}

	enum class LRvalue { null, lvalue, rvalue, rliteral };
	static const mRealType& OpTypeAllowed(TokenType, const mRealType&, const mRealType&);
	static const Token* OpLiteralCal(TokenType, const Token*, const Token*);
};

//类型
struct mRealType {
	mRealType(unique_ptr<mType> type, Op::LRvalue lrvalue) :type(move(type)), lrvalue(lrvalue) {};
	Op::LRvalue lrvalue;
	unique_ptr<mType> type;
	void makePointer() { type = make_unique<mTPointer>(move(type)); }
	mType* operator->() const { return type.get(); }
	//复制构造函数，提供深层复制
	/*mRealType(const mRealType& p) {
		type = p.type->clone();
		lrvalue = p.lrvalue;
	}
	//复制赋值运算符，提供深层复制
	mRealType& operator=(const mRealType& p) {
		type = p.type->clone();
		lrvalue = p.lrvalue;
	}*/
};
/*
struct mType {
	virtual ~mType() {}
	virtual mType* clone() const = 0;
	bool operator==(mType& t2) const { return t2._typeid == _typeid; }
	virtual int _typeid() const = 0;
};

struct mTPointer : public mType {
	explicit mTPointer(mType* r) :reference(r), __typeid(r->_typeid() + 32) {}
	~mTPointer() { delete reference; }
	mType* reference;
	int __typeid;
	//复制构造函数，提供深层复制
	mTPointer(const mTPointer& p) :mType(p) {
		reference = p.clone();
		__typeid = p.__typeid;
	}
	//复制赋值运算符，提供深层复制
	mTPointer& operator=(const mTPointer& p) {
		delete reference;
		reference = p.clone();
		__typeid = p.__typeid;
	}
	mTPointer(mTPointer&&) = default;
	mTPointer& operator=(mTPointer&&) = default;
	mType* clone() const override { return new mTPointer(*this); }
	int _typeid() const override { return __typeid; }
	mType* dereference(Op::LRvalue lrvalue) {
		auto p = reference;
		reference = nullptr;
		delete this;
		return p;
	}
};

struct mTBasic : public mType {
	mTBasic(Op::BuiltInType t) : t(t) {}
	const Op::BuiltInType t;
	mType* clone() const override { return new mTBasic(*this); }
	int _typeid() const override { return t; }
};*/

struct mType {
	virtual ~mType() {}
	virtual unique_ptr<mType> clone() const = 0;
	bool operator==(const mType& t2) const { return t2._typeid == _typeid; }
	virtual int _typeid() const = 0;
};

struct mTPointer : public mType {
	explicit mTPointer(unique_ptr<mType> r) :reference(move(r)), __typeid(r->_typeid() + 32) {}
	~mTPointer() {}
	unique_ptr<mType> reference;
	int __typeid;
	//复制构造函数，提供深层复制
	mTPointer(const mTPointer& p) {
		reference = p.reference->clone();
		__typeid = p.__typeid;
	}
	//复制赋值运算符，提供深层复制
	mTPointer& operator=(const mTPointer& p) {
		reference = p.reference->clone();
		__typeid = p.__typeid;
	}
	mTPointer(mTPointer&&) = default;
	mTPointer& operator=(mTPointer&&) = default;
	unique_ptr<mType> clone() const override { return make_unique<mTPointer>(*this); }
	int _typeid() const override { return __typeid; }
	//深层复制
	unique_ptr<mType> dereference(Op::LRvalue lrvalue) { return reference->clone(); }
};

struct mTBasic : public mType {
	mTBasic(Op::BuiltInType t) : t(t) {}
	const Op::BuiltInType t;
	unique_ptr<mType> clone() const override { return make_unique<mTBasic>(*this); }
	int _typeid() const override { return (int)t; }
};

class Token {
public:
	explicit Token(int lineNo) :lineNo(lineNo) {};
	virtual ~Token() {};
	//类型
	virtual Op::TokenType type() = 0;
	//检验
	virtual bool isIdNum() { return false; };
	virtual bool isExprFollow() { return false; };
	//取数
	virtual string getId() { throw(ErrDesignApp("Extract identifier error.")); };
	virtual Op::BuiltInType getType() { throw(ErrDesignApp("Extract Built-in Type error.")); };
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
	Op::TokenType type() override { return Op::TokenType::Number; };
	bool isIdNum() override { return true; };
	string debug_out() override { return to_string(val); };
private:
	int val;
};

//浮点型常量
class Token_Float :public Token {
public:
	Token_Float(int lineNo, float val) :Token(lineNo), val(val) {};
	Op::TokenType type() override { return Op::TokenType::Number; };
	bool isIdNum() override { return true; };
	string debug_out() override { return to_string(val); };
private:
	float val;
};

//字符串常量
class Token_String :public Token {
public:
	Token_String(int lineNo, string val) :Token(lineNo), val(val) {};
	Op::TokenType type() override { return Op::TokenType::Number; };
	string debug_out() override { return "\"" + val + "\""; };
private:
	string val;
};

//标识符，包括变量函数线程以及ins
class Token_Identifier :public Token {
public:
	Token_Identifier(int lineNo, string val) :Token(lineNo), val(val) {};
	Op::TokenType type() override { return Op::TokenType::Identifier; };
	bool isIdNum() override { return true; };
	string getId() override { return val; };
	string debug_out() override { return val; };
private:
	string val;
};

//关键字,运算符,赋值运算符,逗号分号冒号大小括号
class Token_Operator :public Token {
public:
	Token_Operator(int lineNo, Op::TokenType val) :Token(lineNo), val(val) {};
	Op::TokenType type() override { return val; };
	bool isExprFollow() override { return val == Op::TokenType::Colon || val == Op::TokenType::BigKet || val == Op::TokenType::Comma || val == Op::TokenType::Ket || val == Op::TokenType::Semicolon; };
	string debug_out() override { return Op::ToString(val); };
private:
	Op::TokenType val;
};

//内建类型
class Token_KeywordType :public Token_Operator {
public:
	Token_KeywordType(int lineNo, Op::BuiltInType type) :Token_Operator(lineNo, Op::TokenType::Type), val(type) {};
	Op::BuiltInType getType() override { return val; };
	string debug_out() override { return Op::ToString(val); };
private:
	Op::BuiltInType val;
};

//文档结束
class Token_End :public Token {
public:
	explicit Token_End(int lineNo) :Token(lineNo) {};
	Op::TokenType type() override { return Op::TokenType::End; };
	string debug_out() override { return ""; };
};

//error-type
class ExceptionWithLineNo :public exception {
public:
	explicit ExceptionWithLineNo(int lineNo) :lineNo(lineNo) {};
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