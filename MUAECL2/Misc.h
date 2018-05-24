#pragma once
#include <string>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <memory>
#include <exception>
#include <optional>
#include <variant>
using namespace std;

//error-type
class ExceptionWithLineNo :public exception {
public:
	explicit ExceptionWithLineNo(int lineNo) :lineNo(lineNo) {}
	const int lineNo;
};

class ErrDesignApp :public exception {
public:
	ErrDesignApp(const char* what) :s("Design Error :"s + what + " Please Contact shedarshian@gmail.com"s) {}
	virtual const char* what() const throw() { return s.c_str(); }
	const string s;
};

class Token;
namespace Op {
	template<typename T1, typename T2>
	constexpr static const pair<T2, T1> swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	constexpr static const map<T2, T1> swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	enum TokenType { Identifier, Number, LogicalOr, LogicalAnd, Or, And, BitOr, BitXor, BitAnd, EqualTo, NotEqual, Greater, GreaterEqual, Less, LessEqual, Plus, Minus, Times, Divide, Mod, Negative, Not, Deref, Address, Dot, MidBra, MidKet, Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual, Sub, Type, If, Else, While, For, Goto, Break, Continue, Colon, Semicolon, Comma, Bra, Ket, BigBra, BigKet, End };
	enum BuiltInType { type_error, Void, Int, Float, Point };
	enum LiteralType { lInt, lFloat, lString };
	//非终结符
	enum NonTerm { stmt, stmts, subs, subv, vdecl, insv, ini, inif, inia, exprf, expr, types };
	inline static const unordered_set<char> OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };

	using TT = Op::TokenType;
	using BT = Op::BuiltInType;

	//类型
	class mType {
	public:
		mType() :_address(nullptr) {}
		virtual ~mType() {}
		virtual mType* address();
		virtual mType* dereference() = 0;
	protected:
		mType* _address;
	};

	class mTypeBasic :public mType {
	public:
		mTypeBasic(BuiltInType t) :t(t) {}
		mTypeBasic(int t) :t((BuiltInType)t) {}
		mType* dereference() override { throw(ErrDesignApp("mTypeBasic::dereference")); }
	private:
		BuiltInType t;
	};

	class mTypePointer :public mType {
	public:
		mTypePointer(mType* t) :ptr(t) {}
		mType* dereference() override { return ptr; }
	private:
		mType* ptr;
	};

	mType* mType::address() {
		if (_address == nullptr)
			_address = new mTypePointer(this);
		return _address;
	}

	inline static const mTypeBasic _builtInTypeObj[5] = { mTypeBasic(0), mTypeBasic(1), mTypeBasic(2), mTypeBasic(3), mTypeBasic(4) };
	static mType* BuiltInTypeObj(BuiltInType t) { return (mType*)(_builtInTypeObj + (int)t); }

	inline static const map<TokenType, string> OperatorToString = { { TT::Plus, "+" }, { TT::Minus, "-" }, { TT::Times, "*" }, { TT::Divide, "/" }, { TT::Mod, "%" }, { TT::EqualTo, "==" }, { TT::NotEqual, "!=" }, { TT::Less, "<" }, { TT::LessEqual, "<=" }, { TT::Greater, ">" }, { TT::GreaterEqual, ">=" }, { TT::Not, "!" }, { TT::LogicalOr, "||" }, { TT::LogicalAnd, "&&" }, { TT::BitOr, "|" }, { TT::BitAnd, "&" }, { TT::BitXor, "^" }, { TT::Negative, "(-)" }, { TT::Deref, "(*)" }, { TT::Address, "(&)" }, { TT::Dot, "." }, { TT::And, "and" }, { TT::Or, "or" }, { TT::Equal, "=" }, { TT::PlusEqual, "+=" }, { TT::MinusEqual, "-=" }, { TT::TimesEqual, "*=" }, { TT::DividesEqual, "/=" }, { TT::ModEqual, "%=" }, { TT::LogicalOrEqual, "||=" }, { TT::LogicalAndEqual, "&&=" }, { TT::BitOrEqual, "|=" }, { TT::BitAndEqual, "&=" }, { TT::BitXorEqual, "^=" }, { TT::If, "if" }, { TT::Else, "else" }, { TT::For, "for" }, { TT::While, "while" }, { TT::Break, "break" }, { TT::Continue, "continue" }, { TT::Goto, "goto" }, { TT::Sub, "sub" }, { TT::Semicolon, ";" }, { TT::Colon, ":" }, { TT::Bra, "(" }, { TT::Ket, ")" }, { TT::MidBra, "[" }, { TT::MidKet, "]" }, { TT::BigBra, "{" }, { TT::BigKet, "}" }, { TT::Comma, "," } };
	inline static const map<string, TokenType> StringToOperator = swap_map(OperatorToString);
	inline static const map<string, BuiltInType> StringToType = { { "type_error", type_error }, { "void", Void }, { "int", Int }, { "float", Float }, { "point", Point } };
	inline static const map<BuiltInType, string> TypeToString = swap_map(StringToType);

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
		throw(ErrDesignApp("Op::ToType(int)"));
	}

	enum class LRvalue { null, lvalue, rvalue, rliteral };
	template<typename T1, typename T2>
	static const T2 LiteralCal(TokenType typ, const T1& t1, const optional<T1>& t2) {
		switch (typ) {
		case TokenType::Plus:
			return t1 + *t2;
		case TokenType::Minus:
			return t1 - *t2;
		case TokenType::Times:
			return t1 * *t2;
		case TokenType::Divide:
			return t1 / *t2;
		case TokenType::Mod:
			return t1 % *t2;
		case TokenType::Negative:
			return -t1;
		case TokenType::Not:
			return !t1;
		case TokenType::LogicalOr:
			[[fallthrough]];
		case TokenType::Or:
			return t1 || *t2;
		case TokenType::LogicalAnd:
			[[fallthrough]];
		case TokenType::And:
			return t1 && *t2;
		case TokenType::BitOr:
			return t1 | *t2;
		case TokenType::BitXor:
			return t1 ^ *t2;
		case TokenType::BitAnd:
			return t1 & *t2;
		case TokenType::EqualTo:
			return t1 == *t2;
		case TokenType::NotEqual:
			return t1 != *t2;
		case TokenType::Greater:
			return t1 > *t2;
		case TokenType::GreaterEqual:
			return t1 >= *t2;
		case TokenType::Less:
			return t1 < *t2;
		case TokenType::LessEqual:
			return t1 <= *t2;
		case TokenType::Dot:
			throw(ErrDesignApp("LiteralCal->Dot"));
		default:
			throw(ErrDesignApp("LiteralCal"));
		}
	}
	static const Token* OpLiteralCal(TokenType typ, const Token* tl, const Token* tr);
	//static const mType* OpTypeAllowed(TokenType typ, const mType* tl, const mType* tr);
};

using mType = Op::mType;

class Token {
public:
	explicit Token(int lineNo) :lineNo(lineNo) {}
	virtual ~Token() {}
	//类型
	virtual Op::TokenType type() const = 0;
	//检验
	//virtual const bool isIdNum() { return false; }
	virtual bool isExprFollow() const { return false; }
	//取行号
	int getlineNo() const { return lineNo; }
	//取数
	virtual const int* getInt() const { throw(ErrDesignApp("Token::getInt")); }
	virtual const float* getFloat() const { throw(ErrDesignApp("Token::getFloat")); }
	virtual const string* getString() const { throw(ErrDesignApp("Token::getString")); }
	virtual string getId() const { throw(ErrDesignApp("Token::getId")); }
	virtual Op::BuiltInType getType() const { throw(ErrDesignApp("Token::getType")); }
	//debug输出
	virtual string debug_out() const { return to_string(lineNo); }
	friend ostream& operator<< (ostream& stream, Token& token);
private:
	const int lineNo;
};

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}

class Token_Literal :public Token {
public:
	Token_Literal(int lineNo, int val) :Token(lineNo), val(val) {};
	Token_Literal(int lineNo, float val) :Token(lineNo), val(val) {};
	Token_Literal(int lineNo, string val) :Token(lineNo), val(val) {};
	Op::TokenType type() const override { return Op::TokenType::Number; }
	const int* getInt() const override { return get_if<int>(&val); }
	const float* getFloat() const override { return get_if<float>(&val); }
	const string* getString() const override { return get_if<string>(&val); }
private:
	variant<int, float, string> val;
};

//整型常量
/*class Token_Int :public Token {
public:
	Token_Int(int lineNo, int val) :Token(lineNo), val(val) {}
	const Op::TokenType type() override { return Op::TokenType::Number; }
	//const bool isIdNum() override { return true; }
	const int getInt() override { return val; }
	const string debug_out() override { return to_string(val); }
private:
	int val;
};

//浮点型常量
class Token_Float :public Token {
public:
	Token_Float(int lineNo, float val) :Token(lineNo), val(val) {}
	const Op::TokenType type() override { return Op::TokenType::Number; }
	//const bool isIdNum() override { return true; }
	const float getFloat() override { return val; }
	const string debug_out() override { return to_string(val); }
private:
	float val;
};

//字符串常量
class Token_String :public Token {
public:
	Token_String(int lineNo, string val) :Token(lineNo), val(val) {}
	const Op::TokenType type() override { return Op::TokenType::Number; }
	//const bool isIdNum() override { return true; }
	const string getString() override { return val; }
	const string debug_out() override { return "\"" + val + "\""; }
private:
	string val;
};*/

//标识符，包括变量函数线程以及ins
class Token_Identifier :public Token {
public:
	Token_Identifier(int lineNo, string val) :Token(lineNo), val(val) {}
	Op::TokenType type() const override { return Op::TokenType::Identifier; }
	//bool isIdNum() const override { return true; }
	string getId() const override { return val; }
	string debug_out() const override { return val; }
private:
	string val;
};

//关键字,运算符,赋值运算符,逗号分号冒号大小括号
class Token_Operator :public Token {
public:
	Token_Operator(int lineNo, Op::TokenType val) :Token(lineNo), val(val) {}
	Op::TokenType type() const override { return val; }
	bool isExprFollow() const override { return val == Op::TokenType::Colon || val == Op::TokenType::BigKet || val == Op::TokenType::Comma || val == Op::TokenType::Ket || val == Op::TokenType::Semicolon; }
	string debug_out() const override { return Op::ToString(val); }
private:
	Op::TokenType val;
};

//内建类型
class Token_KeywordType :public Token_Operator {
public:
	Token_KeywordType(int lineNo, Op::BuiltInType type) :Token_Operator(lineNo, Op::TokenType::Type), val(type) {}
	Op::BuiltInType getType() const override { return val; }
	string debug_out() const override { return Op::ToString(val); }
private:
	Op::BuiltInType val;
};

//文档结束
class Token_End :public Token {
public:
	explicit Token_End(int lineNo) :Token(lineNo) {}
	Op::TokenType type() const override { return Op::TokenType::End; }
	string debug_out() const override { return ""; }
};

class ErrOpenedString :public ExceptionWithLineNo {
public:
	ErrOpenedString(int lineNo) :ExceptionWithLineNo(lineNo) {}
	virtual const char* what() const throw() { return "Opened string."; }
};

class ErrDoubleDot :public ExceptionWithLineNo {
public:
	ErrDoubleDot(int lineNo) :ExceptionWithLineNo(lineNo) {}
	virtual const char* what() const throw() { return "Double Dot in a float number."; }
};

class ErrUnknownCharacter :public ExceptionWithLineNo {
public:
	ErrUnknownCharacter(int lineNo) :ExceptionWithLineNo(lineNo) {}
	virtual const char* what() const throw() { return "Unknown Character."; }
};

class ErrUnknownOperator :public ExceptionWithLineNo {
public:
	ErrUnknownOperator(int lineNo) :ExceptionWithLineNo(lineNo) {}
	virtual const char* what() const throw() { return "Unknown operator."; }
};

class ErrIllegalToken :public ExceptionWithLineNo {
public:
	ErrIllegalToken(int lineNo, Token* token) :ExceptionWithLineNo(lineNo) {
		s = "illegal token "s + token->debug_out() + "."s;
	}
	virtual const char* what() const throw() { return s.c_str(); }
private:
	string s;
};

class ErrLabelRedefined :public ExceptionWithLineNo {
public:
	ErrLabelRedefined(int lineNo, string label) :ExceptionWithLineNo(lineNo), id("label "s + label + " redefined."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrVarRedeclared :public ExceptionWithLineNo {
public:
	ErrVarRedeclared(int lineNo, string var) :ExceptionWithLineNo(lineNo), id("variable "s + var + " redeclared."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

const Token* Op::OpLiteralCal(TokenType typ, const Token* tl, const Token* tr) {
	if (auto lstr = tl->getString(), rstr = tr->getString(); lstr && rstr) {
		//auto str = Op::LiteralCal<string>(typ, *lstr, make_optional(*rstr));
		if (lstr || rstr)
			throw(ErrDesignApp("OpLiteralCal(string, other)"));
	}
	return nullptr;
}