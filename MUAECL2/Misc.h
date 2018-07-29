#pragma once
#include <string>
#include <map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <memory>
#include <exception>
#include <optional>
#include <variant>
#include <functional>
#define TYPE(name) Op::mType::name
#define VTYPE(name, lr, ...) Op::mVType{ TYPE(name), Op::LRvalue::lr##value, ##__VA_ARGS__ }
#define OFFSET 128
using namespace std;

class Token;
class tNoVars;
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
	enum TokenType { Identifier, Number, LogicalOr, LogicalAnd, Or, And, BitOr, BitXor, BitAnd, EqualTo, NotEqual, Greater, GreaterEqual, Less, LessEqual, Plus, Minus, Times, Divide, Mod, Negative, Not, Deref, Address, Dot, MidBra, MidKet, Equal, PlusEqual, MinusEqual, TimesEqual, DividesEqual, ModEqual, LogicalOrEqual, LogicalAndEqual, BitOrEqual, BitAndEqual, BitXorEqual, Sub, Type, If, Else, While, For, Goto, Break, Continue, Colon, Semicolon, Comma, Bra, Ket, BigBra, BigKet, End };\
	/// built-in types
	enum class mType { type_error, Void, Int, Float, String, Point, inilist };
	/// nonterminators used for parser
	enum NonTerm { stmt, stmts, subs, subv, vdecl, insv, ini, inif, inia, exprf, expr };
	
	class Ch {
		/// <summary>changing enums to string</summary>
	public:
		static const unordered_set<char> OperatorChar;
		static const map<TokenType, string> OperatorToString;
		static const map<string, TokenType> StringToOperator;
		static const map<string, mType> StringToType;
		static const map<mType, string> TypeToString;

		static string ToString(TokenType op);
		static TokenType ToOperator(string s);
		static string ToString(mType op);
		static mType ToType(string s);
		static NonTerm ToType(int id);
	};

	/// value type, l and r
	enum LRvalue { lvalue, rvalue };
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
		/// used as rank for type changing
		enum { LTOR = 1, ITOF = 5 };
		/// saves : operator->left operand + right operand + return type + operator reload ID
		static const multimap<TokenType, tuple<mVType, mVType, mVType, int>> typeChange;
		/// <summary>implicit type change, return ranks</summary>
		/// <returns>return rank for changes, saves what changes are done. can change to object if needs</returns>
		static int canChangeTo(const mVType& typ, const mVType& typto);
	};
};

class Token {
public:
	explicit Token(int lineNo);
	virtual ~Token() = default;
	//����
	virtual Op::TokenType type() const = 0;
	//����
	virtual bool isExprFollow() const;
	//ȡ�к�
	int getlineNo() const;
	//ȡ��
	virtual int* getInt();
	virtual float* getFloat();
	virtual string* getString();
	virtual string getId() const;
	virtual Op::mType getType() const;
	//debug���
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

//��ʶ�����������������߳��Լ�ins
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

//�ؼ���,�����,��ֵ�����,���ŷֺ�ð�Ŵ�С����
class Token_Operator :public Token {
public:
	Token_Operator(int lineNo, Op::TokenType val);
	Op::TokenType type() const override;
	bool isExprFollow() const override;
	string debug_out() const override;
private:
	Op::TokenType val;
};

//�ڽ�����
class Token_KeywordType :public Token_Operator {
public:
	Token_KeywordType(int lineNo, Op::mType type);
	Op::mType getType() const override;
	string debug_out() const override;
private:
	Op::mType val;
};

//�ĵ�����
class Token_End :public Token {
public:
	explicit Token_End(int lineNo);
	Op::TokenType type() const override;
	string debug_out() const override;
};

class ReadIns {
public:
	enum class NumType { Anything, Int, Float, String, Call };
	static map<string, pair<int, vector<NumType>>> ins;			//ins��
	static map<pair<int, vector<NumType>>, pair<int, int>> insDeltaStackptr;//ֱ�ӷ��ʶ�ջ��ins�Զ�ջָ���Ӱ�죬ֵ��firstΪpop����secondΪpush����
	static map<string, pair<int, vector<NumType>>> mode;		//mode��
	static map<string, pair<int, NumType>> globalVariable;		//ȫ�ֱ�����
	static map<string, int> constint;							//���ų�����
	static map<string, float> constfloat;
	static set<string> integratedFunction;						//���ɺ�����
	static set<string> defaultList;								//default.ecl�е��߳�����

	//��ȡins.ini��default.ini
	// TODO: Fill insDeltaStackptr.
	// TODO: Support NumType::Call.
	static void Read();
};

//error-type
class ExceptionWithLineNo :public exception {
public:
	explicit ExceptionWithLineNo(int lineNo) : lineNo(lineNo) {}
	const int lineNo;
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
		case -5: id = "Unexpected " + tokenName + ", unclosed braket."; break;
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
		case -33: id = "Unexpected " + tokenName + ", expect left braket."; break;
		case -34: id = "Unexpected " + tokenName + ", expect file end."; break;
		default: break;
		}
	}
	virtual const char* what() const throw() { return id.c_str(); }
private:
	string id;
};