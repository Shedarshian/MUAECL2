#include "stdafx.h"
#include <functional>
#include "Misc.h"
#include "GrammarTree.h"
using namespace std;
using namespace Op;
using TT = Op::TokenType;
using BT = Op::mType;

const unordered_set<char> Op::Ch::OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };
const map<TokenType, string> Op::Ch::OperatorToString = { { TT::Plus, "+" }, { TT::Minus, "-" }, { TT::Times, "*" }, { TT::Divide, "/" }, { TT::Mod, "%" }, { TT::EqualTo, "==" }, { TT::NotEqual, "!=" }, { TT::Less, "<" }, { TT::LessEqual, "<=" }, { TT::Greater, ">" }, { TT::GreaterEqual, ">=" }, { TT::Not, "!" }, { TT::LogicalOr, "||" }, { TT::LogicalAnd, "&&" }, { TT::BitOr, "|" }, { TT::BitAnd, "&" }, { TT::BitXor, "^" }, { TT::Negative, "(-)" }, { TT::Deref, "(*)" }, { TT::Address, "(&)" }, { TT::Dot, "." }, { TT::And, "and" }, { TT::Or, "or" }, { TT::Equal, "=" }, { TT::PlusEqual, "+=" }, { TT::MinusEqual, "-=" }, { TT::TimesEqual, "*=" }, { TT::DividesEqual, "/=" }, { TT::ModEqual, "%=" }, { TT::LogicalOrEqual, "||=" }, { TT::LogicalAndEqual, "&&=" }, { TT::BitOrEqual, "|=" }, { TT::BitAndEqual, "&=" }, { TT::BitXorEqual, "^=" }, { TT::If, "if" }, { TT::Else, "else" }, { TT::For, "for" }, { TT::While, "while" }, { TT::Break, "break" }, { TT::Continue, "continue" }, { TT::Goto, "goto" }, { TT::Sub, "sub" }, { TT::Semicolon, ";" }, { TT::Colon, ":" }, { TT::Bra, "(" }, { TT::Ket, ")" }, { TT::MidBra, "[" }, { TT::MidKet, "]" }, { TT::BigBra, "{" }, { TT::BigKet, "}" }, { TT::Comma, "," } };
const map<string, TokenType> Op::Ch::StringToOperator = Op::swap_map(Op::Ch::OperatorToString);
const map<string, mType> Op::Ch::StringToType = { { "type_error", BT::type_error }, { "void", BT::Void }, { "int", BT::Int }, { "float", BT::Float }, { "point", BT::Point }, { "initializer_list", BT::inilist } };
const map<mType, string> Op::Ch::TypeToString = Op::swap_map(Op::Ch::StringToType);

string Op::Ch::ToString(TokenType op) { return OperatorToString.find(op)->second; }
TokenType Op::Ch::ToOperator(string s) { return StringToOperator.find(s)->second; }
string Op::Ch::ToString(mType op) { return TypeToString.find(op)->second; }
mType Op::Ch::ToType(string s) { return StringToType.find(s)->second; }
NonTerm Op::Ch::ToType(int id) {
	if (id >= 2 && id <= 11) return NonTerm::stmt;
	if (id >= 22 && id <= 28) return NonTerm::expr;
	if (id == 12) return NonTerm::inia;
	if (id == 14 || id == 15) return NonTerm::inif;
	if (id == 16 || id == 17) return NonTerm::ini;
	if (id == 20 || id == 21) return NonTerm::exprf;
	throw(ErrDesignApp("Op::ToType(int)"));
}

const multimap<TokenType, tuple<mVType, mVType, mVType, int>> makeTypeChange() {
	multimap<TokenType, tuple<mVType, mVType, mVType, int>> t;
	for (auto tt : { Op::LogicalOr, Op::LogicalAnd, Op::Or, Op::And, Op::BitOr, Op::BitXor, Op::BitAnd, Op::Mod }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, r), VTYPE(Int, r), VTYPE(Int, r), (int)tt));
	}
	for (auto tt : { Op::EqualTo, Op::NotEqual, Op::Greater, Op::GreaterEqual, Op::Less, Op::LessEqual }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, r), VTYPE(Int, r), VTYPE(Int, r), (int)tt));
		//float & float = int, id+OFFSET
		t.emplace(tt, make_tuple(VTYPE(Float, r), VTYPE(Float, r), VTYPE(Int, r), (int)tt + OFFSET));
		//point & point = int, id+2*OFFSET
		t.emplace(tt, make_tuple(VTYPE(Point, r), VTYPE(Point, r), VTYPE(Int, r), (int)tt + OFFSET * 2));
		//string & string = int, id+3*OFFSET
		t.emplace(tt, make_tuple(VTYPE(String, r), VTYPE(String, r), VTYPE(Int, r), (int)tt + OFFSET * 3));
	}
	for (auto tt : { Op::Plus, Op::Minus, Op::Colon }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, r), VTYPE(Int, r), VTYPE(Int, r), (int)tt));
		//float & float = float, id+OFFSET
		t.emplace(tt, make_tuple(VTYPE(Float, r), VTYPE(Float, r), VTYPE(Float, r), (int)tt + OFFSET));
		//point & point = point, id+2*OFFSET
		t.emplace(tt, make_tuple(VTYPE(Point, r), VTYPE(Point, r), VTYPE(Point, r), (int)tt + OFFSET * 2));
		//string & string = string, id+3*OFFSET
		t.emplace(tt, make_tuple(VTYPE(String, r), VTYPE(String, r), VTYPE(String, r), (int)tt + OFFSET * 3));
		//inilist & inilist = inilist, id+4*OFFSET
		if (tt == Op::Colon)
			t.emplace(tt, make_tuple(VTYPE(inilist, r), VTYPE(inilist, r), VTYPE(inilist, r), (int)tt + OFFSET * 3));
	}
	for (auto tt : { Op::Times, Op::Divide }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, r), VTYPE(Int, r), VTYPE(Int, r), (int)tt));
		//float & float = float, id+OFFSET
		t.emplace(tt, make_tuple(VTYPE(Float, r), VTYPE(Float, r), VTYPE(Float, r), (int)tt + OFFSET));
		//point & float = point, id+2*OFFSET
		t.emplace(tt, make_tuple(VTYPE(Point, r), VTYPE(Float, r), VTYPE(Point, r), (int)tt + OFFSET * 2));
		//float * point = point, id+3*OFFSET
		if (tt == Op::Times)
			t.emplace(tt, make_tuple(VTYPE(Float, r), VTYPE(Point, r), VTYPE(Point, r), (int)tt + OFFSET * 3));
	}
	for (auto tt : { Op::Negative, Op::Not }) {
		//int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, r), VTYPE(type_error, r), VTYPE(Int, r), (int)tt));
		//float = float, id+OFFSET
		t.emplace(tt, make_tuple(VTYPE(Float, r), VTYPE(type_error, r), VTYPE(Float, r), (int)tt + OFFSET));
		//point = point, id+OFFSET*2
		t.emplace(tt, make_tuple(VTYPE(Point, r), VTYPE(type_error, r), VTYPE(Point, r), (int)tt + OFFSET * 2));
	}
	//int = void l, id+0
	t.emplace(Op::Deref, make_tuple(VTYPE(Int, r), VTYPE(type_error, r), VTYPE(Void, l), (int)Op::Deref));
	//address, no
	//midbra, no
	//dot, TODO
	for (auto tt : { Op::Equal, Op::PlusEqual, Op::MinusEqual }) {
		//int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, l), VTYPE(Int, r), VTYPE(Void, r), (int)tt));
		//float = float, id+OFFSET
		t.emplace(tt, make_tuple(VTYPE(Float, l), VTYPE(Float, r), VTYPE(Void, r), (int)tt + OFFSET));
		//point = point, id+2*OFFSET
		t.emplace(tt, make_tuple(VTYPE(Point, l), VTYPE(Point, r), VTYPE(Void, r), (int)tt + OFFSET * 2));
		//string = string, id+3*OFFSET
		t.emplace(tt, make_tuple(VTYPE(String, l), VTYPE(String, r), VTYPE(Void, r), (int)tt + OFFSET * 3));
		//point = inilist, id+4*OFFSET, 是否需要特殊检查？
		if (tt == Op::Equal)
			t.emplace(tt, make_tuple(VTYPE(Point, l), VTYPE(inilist, r), VTYPE(Void, r), (int)tt + OFFSET * 4));
	}
	for (auto tt : { Op::TimesEqual, Op::DividesEqual }) {
		//int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, l), VTYPE(Int, r), VTYPE(Void, r), (int)tt));
		//float = float, id+OFFSET
		t.emplace(tt, make_tuple(VTYPE(Float, l), VTYPE(Float, r), VTYPE(Void, r), (int)tt + OFFSET));
		//point = float, id+2*OFFSET
		t.emplace(tt, make_tuple(VTYPE(Point, l), VTYPE(Float, r), VTYPE(Void, r), (int)tt + OFFSET * 2));
	}
	for (auto tt : { Op::ModEqual, Op::LogicalOrEqual, Op::LogicalAndEqual, Op::BitOrEqual, Op::BitAndEqual, Op::BitXorEqual }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(VTYPE(Int, l), VTYPE(Int, r), VTYPE(Void, r), (int)tt));
	}
	return t;
}
const multimap<TokenType, tuple<mVType, mVType, mVType, int>> Op::mVType::typeChange = makeTypeChange();

int Op::mVType::canChangeTo(const mVType& typ, const mVType& typto){
	int rank = 0;
	//左值到右值转换
	if (typ.valuetype == typto.valuetype || (typ.valuetype == Op::lvalue && (rank += LTOR)))
		//整数到浮点转换
		if (typ.type == typto.type || (typ.type == Op::mType::Int && typto.type == Op::mType::Float && (rank += ITOF)))
			return rank;
	return -1;
}

Token::Token(int lineNo) :lineNo(lineNo) {}
bool Token::isExprFollow() const { return false; }
int Token::getlineNo() const { return lineNo; }
int* Token::getInt() { throw(ErrDesignApp("Token::getInt")); }
float* Token::getFloat() { throw(ErrDesignApp("Token::getFloat")); }
string* Token::getString() { throw(ErrDesignApp("Token::getString")); }
string Token::getId() const { throw(ErrDesignApp("Token::getId")); }
Op::mType Token::getType() const { throw(ErrDesignApp("Token::getType")); }
string Token::debug_out() const { return to_string(lineNo); }

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}

Token_Literal::Token_Literal(int lineNo, int val) :Token(lineNo), val(val) {}
Token_Literal::Token_Literal(int lineNo, float val) :Token(lineNo), val(val) {}
Token_Literal::Token_Literal(int lineNo, string val) :Token(lineNo), val(val) {}
Op::TokenType Token_Literal::type() const { return Op::TokenType::Number; }
int* Token_Literal::getInt() { return get_if<int>(&val); }
float* Token_Literal::getFloat() { return get_if<float>(&val); }
string* Token_Literal::getString() { return get_if<string>(&val); }
string Token_Literal::debug_out() const {
	if (auto i = get_if<int>(&val))
		return to_string(*i);
	else if (auto f = get_if<float>(&val))
		return to_string(*f);
	else
		return *get_if<string>(&val);
}

Token_Identifier::Token_Identifier(int lineNo, string val) :Token(lineNo), val(val) {}
Op::TokenType Token_Identifier::type() const { return Op::TokenType::Identifier; }
//bool Token_Identifier::VTYPE(IdNum, r)() const { return true; }
string Token_Identifier::getId() const { return val; }
string Token_Identifier::debug_out() const { return val; }

Token_Operator::Token_Operator(int lineNo, Op::TokenType val) :Token(lineNo), val(val) {}
Op::TokenType Token_Operator::type() const { return val; }
bool Token_Operator::isExprFollow() const {
	return val == Op::TokenType::Colon || val == Op::TokenType::BigKet || val == Op::TokenType::Comma || val == Op::TokenType::Ket || val == Op::TokenType::Semicolon;
}
string Token_Operator::debug_out() const { return Op::Ch::ToString(val); }

Token_KeywordType::Token_KeywordType(int lineNo, Op::mType type) :Token_Operator(lineNo, Op::TokenType::Type), val(type) {}
Op::mType Token_KeywordType::getType() const { return val; }
string Token_KeywordType::debug_out() const { return Op::Ch::ToString(val); }

Token_End::Token_End(int lineNo) :Token(lineNo) {}
Op::TokenType Token_End::type() const { return Op::TokenType::End; }
string Token_End::debug_out() const { return ""; }
