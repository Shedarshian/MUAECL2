#include "stdafx.h"
#include <functional>
#include "Misc.h"
#include "GrammarTree.h"
using namespace std;
using namespace Op;
using TT = Op::TokenType;
using BT = Op::BuiltInType;

mType* mType::address() {
	if (_address == nullptr)
		_address = new mTypePointer(this);
	return _address;
}

mTypeBasic::mTypeBasic(BuiltInType t) :t(t) {}
mTypeBasic::mTypeBasic(int t) :t((BuiltInType)t) {}
mType* mTypeBasic::dereference() { return nullptr; }
BuiltInType mTypeBasic::get() { return t; }
string mTypeBasic::ToString() { return Op::Ch::ToString(t); }

/*mTypeLiteral::mTypeLiteral(LiteralType t) : t(t) {}
mType* mTypeLiteral::address() { throw(ErrDesignApp("mTypeLiteral::address")); }
mType* mTypeLiteral::dereference() { return nullptr; }
LiteralType mTypeLiteral::get() { return t; }	
string mTypeLiteral::ToString() { return "literal"s; }*/

mTypePointer::mTypePointer(mType* t) :ptr(t) {}
mType* mTypePointer::dereference() { return ptr; }
string mTypePointer::ToString() { return ptr->ToString() + '*'; }

const unordered_set<char> Op::Ch::OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };
const map<TokenType, string> Op::Ch::OperatorToString = { { TT::Plus, "+" }, { TT::Minus, "-" }, { TT::Times, "*" }, { TT::Divide, "/" }, { TT::Mod, "%" }, { TT::EqualTo, "==" }, { TT::NotEqual, "!=" }, { TT::Less, "<" }, { TT::LessEqual, "<=" }, { TT::Greater, ">" }, { TT::GreaterEqual, ">=" }, { TT::Not, "!" }, { TT::LogicalOr, "||" }, { TT::LogicalAnd, "&&" }, { TT::BitOr, "|" }, { TT::BitAnd, "&" }, { TT::BitXor, "^" }, { TT::Negative, "(-)" }, { TT::Deref, "(*)" }, { TT::Address, "(&)" }, { TT::Dot, "." }, { TT::And, "and" }, { TT::Or, "or" }, { TT::Equal, "=" }, { TT::PlusEqual, "+=" }, { TT::MinusEqual, "-=" }, { TT::TimesEqual, "*=" }, { TT::DividesEqual, "/=" }, { TT::ModEqual, "%=" }, { TT::LogicalOrEqual, "||=" }, { TT::LogicalAndEqual, "&&=" }, { TT::BitOrEqual, "|=" }, { TT::BitAndEqual, "&=" }, { TT::BitXorEqual, "^=" }, { TT::If, "if" }, { TT::Else, "else" }, { TT::For, "for" }, { TT::While, "while" }, { TT::Break, "break" }, { TT::Continue, "continue" }, { TT::Goto, "goto" }, { TT::Sub, "sub" }, { TT::Semicolon, ";" }, { TT::Colon, ":" }, { TT::Bra, "(" }, { TT::Ket, ")" }, { TT::MidBra, "[" }, { TT::MidKet, "]" }, { TT::BigBra, "{" }, { TT::BigKet, "}" }, { TT::Comma, "," } };
const map<string, TokenType> Op::Ch::StringToOperator = Op::swap_map(Op::Ch::OperatorToString);
const map<string, BuiltInType> Op::Ch::StringToType = { { "type_error", BT::type_error }, { "void", BT::Void }, { "int", BT::Int }, { "float", BT::Float }, { "point", BT::Point }, { "initializer_list", BT::inilist } };
const map<BuiltInType, string> Op::Ch::TypeToString = Op::swap_map(Op::Ch::StringToType);

string Op::Ch::ToString(TokenType op) { return OperatorToString.find(op)->second; }
TokenType Op::Ch::ToOperator(string s) { return StringToOperator.find(s)->second; }
string Op::Ch::ToString(BuiltInType op) { return TypeToString.find(op)->second; }
BuiltInType Op::Ch::ToType(string s) { return StringToType.find(s)->second; }
string Op::Ch::ToString(mType* type) { return type->ToString(); }
NonTerm Op::Ch::ToType(int id) {
	if (id >= 2 && id <= 11) return NonTerm::stmt;
	if (id >= 22 && id <= 28) return NonTerm::expr;
	if (id == 12) return NonTerm::inia;
	if (id == 14 || id == 15) return NonTerm::inif;
	if (id == 16 || id == 17) return NonTerm::ini;
	if (id == 20 || id == 21) return NonTerm::exprf;
	throw(ErrDesignApp("Op::ToType(int)"));
}

using ftt = function<bool(mVType&, mVType&)>;
using fttt = function<mVType(mVType&, mVType&)>;
/*const map<TokenType, function<opp(mVType, mVType)>> makeTypeChange() {
	#define OFFSET 128
	using namespace std::placeholders;
	map<TokenType, function<opp(mVType, mVType)>> t;
	for (auto tt : { Op::LogicalOr, Op::LogicalAnd, Op::Or, Op::And, Op::BitOr, Op::BitXor, Op::BitAnd, Op::Mod }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != rvalue || tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type && tl.type == TYPE(Int))
				//int & int = int, id+0
				return make_optional(make_pair((int)tt, VTYPE(Int, r)));
			return opp{};
		};
	}
	for (auto tt : { Op::EqualTo, Op::NotEqual, Op::Greater, Op::GreaterEqual, Op::Less, Op::LessEqual }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != rvalue || tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type) {
				if (tl.type == TYPE(Int))
					//int & int = int, id+0
					return make_optional(make_pair((int)tt, VTYPE(Int, r)));
				else if (tl.type == TYPE(Float))
					//float & float = int, id+OFFSET
					return make_optional(make_pair((int)tt + OFFSET, VTYPE(Int, r)));
				else if (tl.type == TYPE(Point))
					//point & point = int, id+2*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 2, VTYPE(Int, r)));
				else if (tl.type == TYPE(String))
					//string & string = int, id+3*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 3, VTYPE(Int, r)));
				else if (tl->isPointer())
					//T* & T* = int, id+4*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 4, VTYPE(Int, r)));
			}
			return opp{};
		};
	}
	for (auto tt : { Op::Plus, Op::Minus }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != rvalue || tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type) {
				if (tl.type == TYPE(Int))
					//int & int = int, id+0
					return make_optional(make_pair((int)tt, VTYPE(Int, r)));
				else if (tl.type == TYPE(Float))
					//float & float = float, id+OFFSET
					return make_optional(make_pair((int)tt + OFFSET, VTYPE(Float, r)));
				else if (tl.type == TYPE(Point))
					//point & point = point, id+2*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 2, VTYPE(Point, r)));
				else if (tt == Op::Plus && tl.type == TYPE(String))
					//string & string = string, id+3*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 3, VTYPE(String, r)));
			}
			else if (tl->isPointer() && tr.type == TYPE(Int)) {
				//T* & int = T*, id+4*OFFSET
				return make_optional(make_pair((int)tt + OFFSET * 4, mVType{ tl.type, rvalue }));
			}
			else if (tt == Op::Plus && tr->isPointer() && tl.type == TYPE(Int)) {
				//int & T* = T*, id+5*OFFSET
				return make_optional(make_pair((int)tt + OFFSET * 5, mVType{ tr.type, rvalue }));
			}
			return opp{};
		};
	}
	for (auto tt : { Op::Times, Op::Divide }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != rvalue || tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type) {
				if (tl.type == TYPE(Int))
					//int & int = int, id+0
					return make_optional(make_pair((int)tt, VTYPE(Int, r)));
				else if (tl.type == TYPE(Float))
					//float & float = float, id+OFFSET
					return make_optional(make_pair((int)tt + OFFSET, VTYPE(Float, r)));
			}
			else if (tl.type == TYPE(Point) && tr.type == TYPE(Float)) {
				//point & float = point, id+2*OFFSET
				return make_optional(make_pair((int)tt + OFFSET * 2, VTYPE(Point, r)));
			}
			else if (tr.type == TYPE(Point) && tl.type == TYPE(Int)) {
				//float & point = point, id+3*OFFSET
				return make_optional(make_pair((int)tt + OFFSET * 3, VTYPE(Point, r)));
			}
			return opp{};
		};
	}
	for (auto tt : { Op::Negative, Op::Not }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != rvalue)
				return opp{};
			if (tl.type == TYPE(Int))
				//int = int, id+0
				return make_optional(make_pair((int)tt, VTYPE(Int, r)));
			else if (tl.type == TYPE(Float))
				//float = float, id+OFFSET
				return make_optional(make_pair((int)tt + OFFSET, VTYPE(Float, r)));
			else if (tt == Op::Negative && tl.type == TYPE(Point))
				//point = point, id+OFFSET*2
				return make_optional(make_pair((int)tt + OFFSET * 2, VTYPE(Point, r)));
			return opp{};
		};
	}
	t[Op::Deref] = [tt = Op::Deref](mVType tl, mVType tr) {
		if (tl.valuetype != rvalue)
			return opp{};
		if (tl->isPointer())
			//T* = T, id+0
			return make_optional(make_pair((int)tt, mVType{ tl->dereference(), lvalue }));
		return opp{};
	};
	t[Op::Address] = [tt = Op::Address](mVType tl, mVType tr){
		if (tl.valuetype == lvalue)
			//T = T*, id+0
			return make_optional(make_pair((int)tt, mVType{ tl->address(), rvalue }));
		return opp{};
	};
	t[Op::Dot] = [tt = Op::Dot](mVType tl, mVType tr){
		//TODO
		return opp{};
	};
	t[Op::Equal] = [tt = Op::Equal](mVType tl, mVType tr){
		if (tl.valuetype != lvalue || tr.valuetype != rvalue)
			return opp{};
		if (tl.type == tr.type)
			return make_optional(make_pair((int)tt, VTYPE(Void, r)));
		else if (tl.type == TYPE(Point) && tr.type == TYPE(inilist))
			return make_optional(make_pair((int)tt + OFFSET, VTYPE(Void, r)));
		else if (tl.type == TYPE(Void) && tl.isLiteral)
			return make_optional(make_pair((int)tt + 2 * OFFSET, VTYPE(Void, r)));
		return opp{};
	};
	for (auto tt : { Op::PlusEqual, Op::MinusEqual }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != lvalue && tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type) {
				if (tl.type == TYPE(Int))
					//int = int, id+0
					return make_optional(make_pair((int)tt, VTYPE(Void, r)));
				else if (tl.type == TYPE(Float))
					//float = float, id+OFFSET
					return make_optional(make_pair((int)tt + OFFSET, VTYPE(Void, r)));
				else if (tl.type == TYPE(Point))
					//point = point, id+2*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 2, VTYPE(Void, r)));
				else if (tt == Op::PlusEqual && tl.type == TYPE(String))
					//string = string, id+3*OFFSET
					return make_optional(make_pair((int)tt + OFFSET * 3, VTYPE(Void, r)));
			}
			else if (tl->isPointer() && tr.type == TYPE(Int))
				//T* = int, id+4*OFFSET
				return make_optional(make_pair((int)tt + OFFSET * 4, VTYPE(Void, r)));
			return opp{};
		};
	}
	for (auto tt : { Op::TimesEqual, Op::DividesEqual }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != lvalue && tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type) {
				if (tl.type == TYPE(Int))
					//int = int, id+0
					return make_optional(make_pair((int)tt, VTYPE(Void, r)));
				else if (tl.type == TYPE(Float))
					//float = float, id+OFFSET
					return make_optional(make_pair((int)tt + OFFSET, VTYPE(Void, r)));
			}
			else if (tl.type == TYPE(Point) && tr.type == TYPE(Float))
				//point = float, id+2*OFFSET
				return make_optional(make_pair((int)tt + OFFSET * 2, VTYPE(Void, r)));
			return opp{};
		};
	}
	for (auto tt : { Op::ModEqual, Op::LogicalOrEqual, Op::LogicalAndEqual, Op::BitOrEqual, Op::BitAndEqual, Op::BitXorEqual }) {
		t[tt] = [tt](mVType tl, mVType tr) {
			if (tl.valuetype != lvalue || tr.valuetype != rvalue)
				return opp{};
			if (tr.type == tl.type && tl.type == TYPE(Int))
				//int & int = int, id+0
				return make_optional(make_pair((int)tt, VTYPE(Void, r)));
			return opp{};
		};
	}
	return t;
	#undef OFFSET
}*/
const multimap<TokenType, tuple<ftt*, ftt*, fttt*, int>> makeTypeChange() {
	#define OFFSET 128
	using namespace std::placeholders;
	auto isSame = new auto([](mVType& t1, mVType&, mVType tfix) { return t1 == tfix; });
	auto isInt = new ftt(bind(*isSame, _1, _2, VTYPE(Int, r)));
	auto isFloat = new ftt(bind(*isSame, _1, _2, VTYPE(Float, r)));
	auto isPoint = new ftt(bind(*isSame, _1, _2, VTYPE(Point, r)));
	auto isString = new ftt(bind(*isSame, _1, _2, VTYPE(String, r)));
	auto isInilist = new ftt(bind(*isSame, _1, _2, VTYPE(inilist, r)));
	auto isPointer = new ftt([](mVType& t1, mVType&) { return t1->isPointer() && t1.valuetype == rvalue; });
	auto isLInt = new ftt(bind(*isSame, _1, _2, VTYPE(Int, l)));
	auto isLFloat = new ftt(bind(*isSame, _1, _2, VTYPE(Float, l)));
	auto isLPoint = new ftt(bind(*isSame, _1, _2, VTYPE(Point, l)));
	auto isLString = new ftt(bind(*isSame, _1, _2, VTYPE(String, l)));
	auto isLPointer = new ftt([](mVType& t1, mVType&) { return t1->isPointer() && t1.valuetype == lvalue; });
	auto LequalR = new ftt([](mVType& t1, mVType& t2) { return t1.type == t2.type && t1.valuetype == rvalue; });
	auto RVoid = new fttt([](mVType&, mVType&) { return VTYPE(Void, r); });
	auto RInt = new fttt([](mVType&, mVType&) { return VTYPE(Int, r); });
	auto RFloat = new fttt([](mVType&, mVType&) { return VTYPE(Float, r); });
	auto RPoint = new fttt([](mVType&, mVType&) { return VTYPE(Point, r); });
	auto RString = new fttt([](mVType&, mVType&) { return VTYPE(String, r); });
	multimap<TokenType, tuple<ftt*, ftt*, fttt*, int>> t;
	for (auto tt : { Op::LogicalOr, Op::LogicalAnd, Op::Or, Op::And, Op::BitOr, Op::BitXor, Op::BitAnd, Op::Mod }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(isInt, isInt, RInt, (int)tt));
	}
	for (auto tt : { Op::EqualTo, Op::NotEqual, Op::Greater, Op::GreaterEqual, Op::Less, Op::LessEqual }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(isInt, isInt, RInt, (int)tt));
		//float & float = int, id+OFFSET
		t.emplace(tt, make_tuple(isFloat, isFloat, RInt, (int)tt + OFFSET));
		//point & point = int, id+2*OFFSET
		t.emplace(tt, make_tuple(isPoint, isPoint, RInt, (int)tt + OFFSET * 2));
		//string & string = int, id+3*OFFSET
		t.emplace(tt, make_tuple(isString, isString, RInt, (int)tt + OFFSET * 3));
		//T* & T* = int, id+4*OFFSET
		t.emplace(tt, make_tuple(isPointer, &equal_to<mVType>::operator(), VTYPE(Int, r), (int)tt + OFFSET * 4));
	}
	for (auto tt : { Op::Plus, Op::Minus }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(isInt, isInt, RInt, (int)tt));
		//float & float = float, id+OFFSET
		t.emplace(tt, make_tuple(isFloat, isFloat, RFloat, (int)tt + OFFSET));
		//point & point = point, id+2*OFFSET
		t.emplace(tt, make_tuple(isPoint, isPoint, RPoint, (int)tt + OFFSET * 2));
		//string & string = string, id+3*OFFSET
		t.emplace(tt, make_tuple(isString, isString, RString, (int)tt + OFFSET * 3));
		//T* & int = T*, id+4*OFFSET
		t.emplace(tt, make_tuple(isPointer, isInt, new fttt([](mVType& t1, mVType&) { return t1; }), (int)tt + OFFSET * 4));
		//int + T* = T*, id+5*OFFSET
		if (tt == Op::Plus)
			t.emplace(tt, make_tuple(isInt, isPointer, new fttt([](mVType&, mVType& t1) { return t1; }), (int)tt + OFFSET * 5));
	}
	for (auto tt : { Op::Times, Op::Divide }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(isInt, isInt, RInt, (int)tt));
		//float & float = float, id+OFFSET
		t.emplace(tt, make_tuple(isFloat, isFloat, RFloat, (int)tt + OFFSET));
		//point & float = point, id+2*OFFSET
		t.emplace(tt, make_tuple(isPoint, isFloat, RPoint, (int)tt + OFFSET * 2));
		//float * point = point, id+3*OFFSET
		if (tt == Op::Times)
			t.emplace(tt, make_tuple(isFloat, isPoint, RPoint, (int)tt + OFFSET * 3));
	}
	for (auto tt : { Op::Negative, Op::Not }) {
		//int = int, id+0
		t.emplace(tt, make_tuple(isInt, nullptr, RInt, (int)tt));
		//float = float, id+OFFSET
		t.emplace(tt, make_tuple(isFloat, nullptr, RFloat, (int)tt + OFFSET));
		//point = point, id+OFFSET*2
		t.emplace(tt, make_tuple(isPoint, nullptr, RPoint, (int)tt + OFFSET * 2));
	}
	//T* = T, id+0
	t.emplace(Op::Deref, make_tuple(isPointer, nullptr, new fttt([](mVType& t1, mVType&) { return mVType{ t1->dereference(), lvalue }; }), (int)Op::Deref));
	//T = T*, id+0
	t.emplace(Op::Address, make_tuple(isPointer, nullptr, new fttt([](mVType& t1, mVType&) { return mVType{ t1->address(), rvalue }; }), (int)Op::Address));
	//dot, TODO

	//T = T, id+0
	t.emplace(Op::Equal, make_tuple(new ftt([](mVType& t1, mVType&) { return t1.valuetype == lvalue; }), LequalR, RVoid, (int)Op::Equal));
	//point = inilist, id+OFFSET
	t.emplace(Op::Equal, make_tuple(isLPoint, isInilist, RVoid, (int)Op::Equal + OFFSET));
	//void literal = anything. id+OFFSET*2
	t.emplace(Op::Equal, make_tuple(new ftt([](mVType& t1, mVType&) { return t1 == VTYPE(Void, l) && t1.isLiteral; }), new ftt([](mVType&, mVType&) { return true; }), RVoid, (int)Op::Equal + 2 * OFFSET));

	for (auto tt : { Op::PlusEqual, Op::MinusEqual }) {
		//int = int, id+0
		t.emplace(tt, make_tuple(isLInt, isInt, RVoid, (int)tt));
		//float = float, id+OFFSET
		t.emplace(tt, make_tuple(isLFloat, isFloat, RVoid, (int)tt + OFFSET));
		//point = point, id+2*OFFSET
		t.emplace(tt, make_tuple(isLPoint, isPoint, RVoid, (int)tt + OFFSET * 2));
		//string = string, id+3*OFFSET
		t.emplace(tt, make_tuple(isLString, isString, RVoid, (int)tt + OFFSET * 3));
		//T* = int, id+4*OFFSET
		t.emplace(tt, make_tuple(isLPointer, isInt, RVoid, (int)tt + OFFSET * 4));
	}
	for (auto tt : { Op::TimesEqual, Op::DividesEqual }) {
		//int = int, id+0
		t.emplace(tt, make_tuple(isLInt, isInt, RVoid, (int)tt));
		//float = float, id+OFFSET
		t.emplace(tt, make_tuple(isLFloat, isFloat, RVoid, (int)tt + OFFSET));
		//point = float, id+2*OFFSET
		t.emplace(tt, make_tuple(isLPoint, isFloat, RVoid, (int)tt + OFFSET * 2));
	}
	for (auto tt : { Op::ModEqual, Op::LogicalOrEqual, Op::LogicalAndEqual, Op::BitOrEqual, Op::BitAndEqual, Op::BitXorEqual }) {
		//int & int = int, id+0
		t.emplace(tt, make_tuple(isLInt, isInt, RVoid, (int)tt));
	}
	return t;
	#undef OFFSET
}

const multimap<TokenType, tuple<function<bool(mVType&, mVType&)>*, function<bool(mVType&, mVType&)>*, function<mVType(mVType&, mVType&)>*, int>> Op::mVType::typeChange = makeTypeChange();
vector<pair<mVType, int>> Op::mVType::canChangeTo(mVType typ) {
	vector<pair<mVType, int>> v{ make_pair(typ, 0) };
	bool ltor = false;
	//如果是左值则可以转换成右值
	if (ltor = (typ.valuetype == lvalue)) {
		v.push_back(make_pair(mVType{ typ.type, rvalue, typ.isLiteral }, LTOR));
	}
	//处理隐式类型转换，允许：int->float, int literal->void* literal, anything*->void*
	if (typ.type == TYPE(Int)) {
		v.push_back(make_pair(mVType{ TYPE(Float), rvalue, typ.isLiteral }, ltor * LTOR + ITOF));
		if (typ.isLiteral)
			v.push_back(make_pair(mVType{ TYPE(Void)->address(), rvalue, true }, ltor * LTOR + ITOVP));
	}
	else if (typ->isPointer()) {
		v.push_back(make_pair(mVType{ TYPE(Void)->address(), rvalue, typ.isLiteral }, ltor * LTOR + PTOVP));
	}
	return v;
}

Token::Token(int lineNo) :lineNo(lineNo) {}
bool Token::isExprFollow() const { return false; }
int Token::getlineNo() const { return lineNo; }
int* Token::getInt() { throw(ErrDesignApp("Token::getInt")); }
float* Token::getFloat() { throw(ErrDesignApp("Token::getFloat")); }
string* Token::getString() { throw(ErrDesignApp("Token::getString")); }
string Token::getId() const { throw(ErrDesignApp("Token::getId")); }
Op::BuiltInType Token::getType() const { throw(ErrDesignApp("Token::getType")); }
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

Token_Identifier::Token_Identifier(int lineNo, string val) :Token(lineNo), val(val) {}
Op::TokenType Token_Identifier::type() const { return Op::TokenType::Identifier; }
//bool Token_Identifier::isIdNum() const { return true; }
string Token_Identifier::getId() const { return val; }
string Token_Identifier::debug_out() const { return val; }

Token_Operator::Token_Operator(int lineNo, Op::TokenType val) :Token(lineNo), val(val) {}
Op::TokenType Token_Operator::type() const { return val; }
bool Token_Operator::isExprFollow() const {
	return val == Op::TokenType::Colon || val == Op::TokenType::BigKet || val == Op::TokenType::Comma || val == Op::TokenType::Ket || val == Op::TokenType::Semicolon;
}
string Token_Operator::debug_out() const { return Op::Ch::ToString(val); }

Token_KeywordType::Token_KeywordType(int lineNo, Op::BuiltInType type) :Token_Operator(lineNo, Op::TokenType::Type), val(type) {}
Op::BuiltInType Token_KeywordType::getType() const { return val; }
string Token_KeywordType::debug_out() const { return Op::Ch::ToString(val); }

Token_End::Token_End(int lineNo) :Token(lineNo) {}
Op::TokenType Token_End::type() const { return Op::TokenType::End; }
string Token_End::debug_out() const { return ""; }
