#pragma once
#include <string>
#include <map>
#include <set>
#include <stack>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <memory>
#include <exception>
#include <optional>
#include <variant>
#include <functional>
#include <bitset>
#include <regex>
#include <vector>
#define TYPE(name) Op::mType::name
#define VTYPE(name, lr, ...) Op::mVType{ TYPE(name), Op::LRvalue::lr##value, ##__VA_ARGS__ }
#define OFFSET 128
using namespace std;

class Token;
class tNoVars;

class ReadIns {
public:
	enum class NumType { Anything, Int, Float, String, EncryptedString, Call };
	static multimap<string, pair<int, vector<NumType>>> ins;			//ins表
	static map<pair<int, vector<NumType>>, pair<int, int>> insDeltaStackptr;//直接访问堆栈的ins对堆栈指针的影响，值的first为pop数，second为push数。
	static map<string, pair<int, vector<NumType>>> mode;		//mode表
	static map<string, pair<int, NumType>> globalVariable;		//全局变量表
	static map<string, int> constint;							//符号常量表
	static map<string, float> constfloat;
	static set<string> defaultList;								//default.ecl中的线程名表
	static vector<pair<regex, string>> include;					//预定义宏

	//读取ins.ini与default.ini
	static void Read();
};

namespace Op {
	template<typename T1, typename T2>
	inline constexpr static const pair<T2, T1> swap_pair(const pair<T1, T2>& p) { return pair<T2, T1>(p.second, p.first); }
	template<typename T1, typename T2>
	inline constexpr static const map<T2, T1> swap_map(const map<T1, T2>& m) {
		map<T2, T1> mo;
		transform(m.cbegin(), m.cend(), inserter(mo, mo.begin()), swap_pair<T1, T2>);
		return mo;
	}
	/// all types of tokens
	enum TokenType { Identifier, Number, LogicalOr, LogicalAnd, Or, And, BitOr, BitXor, BitAnd, EqualTo, NotEqual, Greater, GreaterEqual, Less, LessEqual, Plus, Minus, Times, Divide, Mod, Negative, Not, Deref, Address, Dot, MidBra, MidKet, Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual, Sub, NoOverload, Type, If, Else, While, For, Goto, Break, Continue, Thread, Do, Rawins, Colon, Semicolon, Comma, Bra, Ket, BigBra, BigKet, End };
	/// built-in types
	enum class mType { type_error, Void, Int, Float, String, Point, inilist };
	/// nonterminators used for parser
	enum NonTerm { stmt, stmts, subs, subv, vdecl, insv, ini, inif, inia, exprf, expr, data, insdata };
	
	class Ch {
		/// <summary>changing enums to string</summary>
	public:
		static const unordered_set<char> OperatorChar;
		static const map<TokenType, string> OperatorToString;
		static const map<string, TokenType> StringToOperator;
		static const map<string, mType> StringToType;
		static const map<mType, string> TypeToString;
		static const map<ReadIns::NumType, mType> NumTypeToType;
		static const map<mType, ReadIns::NumType> TypeToNumType;

		static string ToString(TokenType op);
		static TokenType ToOperator(string s);
		static string ToString(mType op);
		static mType ToType(string s);
		static mType ToType(ReadIns::NumType t);
		static ReadIns::NumType ToNumType(mType t);
		static NonTerm ToType(int id);
	};

	/// value type, l and r
	enum LRvalue { lvalue, rvalue };
	struct Rank {
		enum { DISABLE = 0, LTOR = 1, ITOF = 2, VTOI = 3, VTOF = 4, VTOSTR = 5, VTOPOINT = 6, SIZE = 7 };
		bitset<7> rank;
		Rank& set(size_t pos, bool value = true) { rank.set(pos, value); return *this; }
		bool incorrect() const { return rank.test(DISABLE); }
		bool correct() const { return !rank.test(DISABLE); }
		bool test(size_t pos) const { return rank.test(pos); }
		//static const Rank RANK_MAX;
		#define RANK_MAX Op::Rank{ bitset<7>(0xFF) }

		friend Rank operator+(const Rank& rankL, const Rank& rankR);
		Rank& operator+=(const Rank& operand);
		friend bool operator<(const Rank& rankL, const Rank& rankR);
		Rank& operator=(const Rank& operand) = default;
		friend bool operator==(const Rank& rankL, const Rank& rankR);
	};
	struct mVType {
		/// <summary>full type object, contains value type and literal type</summary>
		mVType() = default;
		mType type;
		LRvalue valuetype;
		bool isLiteral;
		string debug_out() {
			return (isLiteral ? "literal "s : ""s) + (valuetype == rvalue ? "right value "s : "left value "s) + Ch::ToString(type);
		}
		/// only compare type and value type
		bool operator==(const mVType& tr) const { return type == tr.type && valuetype == tr.valuetype; }
		/// saves : operator->left operand + right operand + return type + operator reload ID
		static const multimap<TokenType, tuple<mVType, mVType, mVType, int>> typeChange;
		/// <summary>implicit type change, return ranks</summary>
		/// <returns>return rank for changes, saves what changes are done.</returns>
		static Rank canChangeTo(const mVType& typ, const mVType& typto);
		/// saves : name->return type + operand + ins ID
		static const multimap<string, tuple<mVType, vector<mVType>, int>> internalFunction;
	};
};

class Token {
public:
	explicit Token(int lineNo);
	virtual ~Token() = default;
	//类型
	virtual Op::TokenType type() const = 0;
	//检验
	virtual bool isExprFollow() const;
	//取行号
	int getlineNo() const;
	//取数
	virtual int* getInt();
	virtual float* getFloat();
	virtual string* getString();
	virtual string getId() const;
	virtual Op::mType getType() const;
	//debug输出
	virtual string debug_out() const;
	friend ostream& operator<< (ostream& stream, Token& token);
private:
	const int lineNo;
};

class Token_Literal :public Token {
public:
	Token_Literal(int lineNo, int val);
	Token_Literal(int lineNo, float val);
	Token_Literal(int lineNo, string val);
	Op::TokenType type() const override;
	int* getInt() override;
	float* getFloat() override;
	string* getString() override;
	string debug_out() const override;
private:
	variant<int, float, string> val;
};

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
	Token_KeywordType(int lineNo, Op::mType type);
	Op::mType getType() const override;
	string debug_out() const override;
private:
	Op::mType val;
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
	explicit ExceptionWithLineNo(int lineNo) : lineNo(lineNo) {}
	const int lineNo;
};

class ErrFileNotFound :public exception {
public:
	ErrFileNotFound(const char* what) : s("File "s + what + " Not Found!"s) {}
	ErrFileNotFound(const string what) : s("File "s + what + " Not Found!"s) {}
	virtual const char* what() const throw() { return s.c_str(); }
private:
	const string s;
};

class ErrDesignApp :public exception {
public:
	ErrDesignApp(const char* what) : s("Design Error :"s + what + " Please Contact shedarshian@gmail.com"s) {}
	ErrDesignApp(const string what) : s("Design Error :"s + what + " Please Contact shedarshian@gmail.com"s) {}
	virtual const char* what() const throw() { return s.c_str(); }
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
	ErrTypeChangeLoss(int lineNo, Op::mVType type, Op::mVType expectType) :ExceptionWithLineNo(lineNo),
		id("error implicit conversing type "s + type.debug_out() + " to type "s + expectType.debug_out() + ". maybe missing explicit conversing."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrDiscardValue :public ExceptionWithLineNo {
public:
	ErrDiscardValue(int lineNo) :ExceptionWithLineNo(lineNo) {};
	virtual const char* what() const throw() { return "discarded value unallowed. If pushing into stack, please use ins_44 and ins_45."; }
};

class ErrVarNotFound :public ExceptionWithLineNo {
public:
	ErrVarNotFound(int lineNo, string var) :ExceptionWithLineNo(lineNo), id("variable "s + var + " not found."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrFuncNotFound :public ExceptionWithLineNo {
public:
	ErrFuncNotFound(int lineNo, string var) :ExceptionWithLineNo(lineNo), id("function "s + var + " not found."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrFuncPara :public ExceptionWithLineNo {
public:
	ErrFuncPara(int lineNo, string var) :ExceptionWithLineNo(lineNo), id("function "s + var + " using wrong parameters."s) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrTypeOperate :public ExceptionWithLineNo {
public:
	ErrTypeOperate(int lineNo, Op::mVType ltype, Op::mVType rtype, string token) :ExceptionWithLineNo(lineNo),
		id("can't use " + token + " on type " + ltype.debug_out() + " and " + rtype.debug_out() + " .") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrParsing :public ExceptionWithLineNo {
public:
	ErrParsing(int lineNo, int ErrNo, string tokenName) :ExceptionWithLineNo(lineNo) {
		switch (ErrNo) {
		case -1: id = "Unexpected " + tokenName + ", expect sub declare."; break;
		case -2: id = "Unexpected " + tokenName + ", expect parameter list."; break;
		case -3: id = "Missing type specifier."; break;
		case -4: id = "Missing parameter name."; break;
		case -5: id = "Unexpected " + tokenName + ", unclosed small braket."; break;
		case -6: id = "Unexpected " + tokenName + ", expect code block begin."; break;
		case -7: id = "Missing label name."; break;
		case -8: id = "Blank statement not allowed."; break;
		case -10: id = "Unclosed big braket."; break;
		case -11: id = "Unexpected " + tokenName + ", probably missing identifier."; break;
		case -12: id = "Nesting sub declare not allowed."; break;
		case -13: id = "Floating else without if."; break;
		case -14: id = "Missing Operator."; break;
		case -16: id = "Please don't use keyword " + tokenName + "."; break;
		case -17: id = "Unexpected " + tokenName + ", we don't know what you want to do."; break;
		case -18: id = "Missing semicolon."; break;
		case -19: id = "Redundant " + tokenName + "."; break;
		case -20: id = "Label at block's end not allowed."; break;
		case -21: id = "Can't declare non-variable."; break;
		case -22: id = "Unbalanced braket."; break;
		case -24: id = "Missing comma."; break;
		case -25: id = "Missing assignment sign."; break;
		case -26: id = "Can't use " + tokenName + " for variable initialize."; break;
		case -27: id = "Can't declare array! (At least for this version of MUAECL)."; break;
		case -28: id = "Can't use difficulty switch inside if or while."; break;
		case -29: id = "Unexpected " + tokenName + ", expect code block end."; break;
		case -30: id = "Unexpected " + tokenName + " in initializer list."; break;
		case -31: id = "Unexpected " + tokenName + ", expect initializer list end."; break;
		case -32: id = "Difficulty switch need to have four expression."; break;
		case -33: id = "Unexpected " + tokenName + ", expect left small braket."; break;
		case -34: id = "Unexpected " + tokenName + ", expect file end."; break;
		case -35: id = "Unexpected " + tokenName + ", expect while."; break;
		case -36: id = "Unexpected " + tokenName + ", expect left big braket."; break;
		case -37: id = "Unexpected " + tokenName + ", expect raw data."; break;
		default: break;
		}
	}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrNoOverloadFunction :public ExceptionWithLineNo {
public:
	ErrNoOverloadFunction(int lineNo, string name) :ExceptionWithLineNo(lineNo), id("no overload function " + name + " found.") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrSubstituteDeliminator :public ExceptionWithLineNo {
public:
	ErrSubstituteDeliminator(int lineNo, char deliminator, bool isEnds) :ExceptionWithLineNo(lineNo), id("deliminator '"s + deliminator + "' must not be alphabet or number or underline in #" + (isEnds ? "ends" : "s")) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrSubstituteNoString :public ExceptionWithLineNo {
public:
	ErrSubstituteNoString(int lineNo) :ExceptionWithLineNo(lineNo) {}
	virtual const char* what() const throw() { return "missing replacing string in #s"; }
};

class ErrSubstituteRedeclare :public ExceptionWithLineNo {
public:
	ErrSubstituteRedeclare(int lineNo, string name) :ExceptionWithLineNo(lineNo), id("name " + name + " repeated") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrSubstituteName :public ExceptionWithLineNo {
public:
	ErrSubstituteName(int lineNo, string name) :ExceptionWithLineNo(lineNo), id("name " + name + "contains illegal character") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrEndsNotFound :public ExceptionWithLineNo {
public:
	ErrEndsNotFound(int lineNo, string name, string type) :ExceptionWithLineNo(lineNo), id("name " + name + "not found in #" + type) {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrDeclareIdList :public ExceptionWithLineNo {
public:
	ErrDeclareIdList(int lineNo, string id_list) :ExceptionWithLineNo(lineNo), id("identifier list " + id_list + " illegal.") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrPreprocessNotFound :public ExceptionWithLineNo {
public:
	ErrPreprocessNotFound(int lineNo, string name) :ExceptionWithLineNo(lineNo), id("preprocess command \"" + name + "\"not found.") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};

class ErrIncludeFileNotFound : public ExceptionWithLineNo {
public:
	ErrIncludeFileNotFound(int lineNo, string name) :ExceptionWithLineNo(lineNo), id("include file \"" + name + "\"not found.") {}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};