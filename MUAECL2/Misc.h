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

class Token;
namespace Op {
	template<typename T1, typename T2>
	inline constexpr static const pair<T2, T1> swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	inline constexpr static const map<T2, T1> swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	enum TokenType { Identifier, Number, LogicalOr, LogicalAnd, Or, And, BitOr, BitXor, BitAnd, EqualTo, NotEqual, Greater, GreaterEqual, Less, LessEqual, Plus, Minus, Times, Divide, Mod, Negative, Not, Deref, Address, Dot, MidBra, MidKet, Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual, Sub, Type, If, Else, While, For, Goto, Break, Continue, Colon, Semicolon, Comma, Bra, Ket, BigBra, BigKet, End };
	enum BuiltInType { type_error, Void, Int, Float, Point, inilist };
	enum LiteralType { lInt, lFloat, lString };
	//非终结符
	enum NonTerm { stmt, stmts, subs, subv, vdecl, insv, ini, inif, inia, exprf, expr, types };
	static const unordered_set<char> OperatorChar;

	//类型
	class mType {
	public:
		mType() = default;
		virtual ~mType() = default;
		virtual mType* address();
		virtual mType* dereference() = 0;
		virtual string ToString() = 0;
	protected:
		mType* _address;
	};

	class mTypeBasic :public mType {
	public:
		mTypeBasic(BuiltInType t);
		mTypeBasic(int t);
		mType* dereference() override;
		BuiltInType get();
		inline string ToString() override;
	private:
		BuiltInType t;
	};

	class mTypePointer :public mType {
	public:
		mTypePointer(mType* t);
		mType* dereference() override;
		inline string ToString() override;
	private:
		mType* ptr;
	};

	inline static const mTypeBasic _builtInTypeObj[6] = { mTypeBasic(0), mTypeBasic(1), mTypeBasic(2), mTypeBasic(3), mTypeBasic(4), mTypeBasic(5) };
	inline static mType* BuiltInTypeObj(BuiltInType t) { return (mType*)(_builtInTypeObj + (int)t); }

	static const map<TokenType, string> OperatorToString;
	static const map<string, TokenType> StringToOperator;
	static const map<string, BuiltInType> StringToType;
	static const map<BuiltInType, string> TypeToString;

	static string ToString(TokenType op);
	static TokenType ToOperator(string s);
	static string ToString(BuiltInType op);
	static BuiltInType ToType(string s);
	static string ToString(mType* type);
	static NonTerm ToType(int id);

	enum LRvalue { nullvalue, lvalue, rvalue, rliteral };
	/*template<typename T1, typename T2>
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
	//static const mType* OpTypeAllowed(TokenType typ, const mType* tl, const mType* tr);*/
};

class Token {
public:
	explicit Token(int lineNo);
	virtual ~Token() = default;
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
	Token_Literal(int lineNo, int val);
	Token_Literal(int lineNo, float val);
	Token_Literal(int lineNo, string val);
	Op::TokenType type() const override;
	const int* getInt() const override;
	const float* getFloat() const override;
	const string* getString() const override;
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
	Token_Identifier(int lineNo, string val);
	Op::TokenType type() const override;
	//bool isIdNum() const override;
	string getId() const override;
	string debug_out() const override;
private:
	string val;
};

//关键字,运算符,赋值运算符,逗号分号冒号大小括号
class Token_Operator :public Token {
public:
	Token_Operator(int lineNo, Op::TokenType val);
	Op::TokenType type() const override;
	bool isExprFollow() const override;
	string debug_out() const override;
private:
	Op::TokenType val;
};

//内建类型
class Token_KeywordType :public Token_Operator {
public:
	Token_KeywordType(int lineNo, Op::BuiltInType type);
	Op::BuiltInType getType() const override;
	string debug_out() const override;
private:
	Op::BuiltInType val;
};

//文档结束
class Token_End :public Token {
public:
	explicit Token_End(int lineNo);
	Op::TokenType type() const override;
	string debug_out() const override;
};

//error-type
class ExceptionWithLineNo :public exception {
public:
	explicit ExceptionWithLineNo(int lineNo);
	const int lineNo;
};

class ErrDesignApp :public exception {
public:
	ErrDesignApp(const char* what);
	virtual const char* what() const throw();
private:
	const string s;
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

class ErrFuncRedeclared :public ExceptionWithLineNo {
public:
	ErrFuncRedeclared(int lineNo, string var) :ExceptionWithLineNo(lineNo), id("function "s + var + " redeclared."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrBreakWithoutWhile :public ExceptionWithLineNo {
public:
	ErrBreakWithoutWhile(int lineNo, string _break) :ExceptionWithLineNo(lineNo), id(_break + " can only be used in while or for block."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrTypeChangeLoss :public ExceptionWithLineNo {
public:
	ErrTypeChangeLoss(int lineNo, mType* type, mType* expectType) :ExceptionWithLineNo(lineNo),
		id("error implicit conversing type "s + type->ToString() + " to type "s + expectType->ToString() + ". maybe missing explicit conversing."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrDiscardValue :public ExceptionWithLineNo {
public:
	ErrDiscardValue(int lineNo) :ExceptionWithLineNo(lineNo) {};
	virtual const char* what() const throw() { return "discarded value unallowed. If pushing into stack, please use ins_44 and ins_45."; }
};

/*class ErrVarNotFound :public ExceptionWithLineNo {
public:
	ErrVarNotFound(int lineNo, string var) :ExceptionWithLineNo(lineNo), id("variable "s + var + " not found."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};*/