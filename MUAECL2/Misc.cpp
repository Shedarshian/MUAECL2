#include "stdafx.h"
#include "Misc.h"
using namespace std;
using namespace Op;
using TT = Op::TokenType;
using BT = Op::BuiltInType;

const unordered_set<char> OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };

mType* mType::address() {
	if (_address == nullptr)
		_address = new mTypePointer(this);
	return _address;
}

mTypeBasic::mTypeBasic(BuiltInType t) :t(t) {}
mTypeBasic::mTypeBasic(int t) :t((BuiltInType)t) {}
mType* mTypeBasic::dereference() { return nullptr; }
BuiltInType mTypeBasic::get() { return t; }
string mTypeBasic::ToString() { return Op::ToString(t); }

/*mTypeLiteral::mTypeLiteral(LiteralType t) : t(t) {}
mType* mTypeLiteral::address() { throw(ErrDesignApp("mTypeLiteral::address")); }
mType* mTypeLiteral::dereference() { return nullptr; }
LiteralType mTypeLiteral::get() { return t; }	
string mTypeLiteral::ToString() { return "literal"s; }*/

mTypePointer::mTypePointer(mType* t) :ptr(t) {}
mType* mTypePointer::dereference() { return ptr; }
string mTypePointer::ToString() { return ptr->ToString() + '*'; }

const map<TokenType, string> OperatorToString = { { TT::Plus, "+" }, { TT::Minus, "-" }, { TT::Times, "*" }, { TT::Divide, "/" }, { TT::Mod, "%" }, { TT::EqualTo, "==" }, { TT::NotEqual, "!=" }, { TT::Less, "<" }, { TT::LessEqual, "<=" }, { TT::Greater, ">" }, { TT::GreaterEqual, ">=" }, { TT::Not, "!" }, { TT::LogicalOr, "||" }, { TT::LogicalAnd, "&&" }, { TT::BitOr, "|" }, { TT::BitAnd, "&" }, { TT::BitXor, "^" }, { TT::Negative, "(-)" }, { TT::Deref, "(*)" }, { TT::Address, "(&)" }, { TT::Dot, "." }, { TT::And, "and" }, { TT::Or, "or" }, { TT::Equal, "=" }, { TT::PlusEqual, "+=" }, { TT::MinusEqual, "-=" }, { TT::TimesEqual, "*=" }, { TT::DividesEqual, "/=" }, { TT::ModEqual, "%=" }, { TT::LogicalOrEqual, "||=" }, { TT::LogicalAndEqual, "&&=" }, { TT::BitOrEqual, "|=" }, { TT::BitAndEqual, "&=" }, { TT::BitXorEqual, "^=" }, { TT::If, "if" }, { TT::Else, "else" }, { TT::For, "for" }, { TT::While, "while" }, { TT::Break, "break" }, { TT::Continue, "continue" }, { TT::Goto, "goto" }, { TT::Sub, "sub" }, { TT::Semicolon, ";" }, { TT::Colon, ":" }, { TT::Bra, "(" }, { TT::Ket, ")" }, { TT::MidBra, "[" }, { TT::MidKet, "]" }, { TT::BigBra, "{" }, { TT::BigKet, "}" }, { TT::Comma, "," } };
const map<string, TokenType> StringToOperator = Op::swap_map(Op::OperatorToString);
const map<string, BuiltInType> StringToType = { { "type_error", BT::type_error }, { "void", BT::Void }, { "int", BT::Int }, { "float", BT::Float }, { "point", BT::Point }, { "initializer_list", BT::inilist } };
const map<BuiltInType, string> TypeToString = Op::swap_map(Op::StringToType);

string Op::ToString(TokenType op) { return OperatorToString.find(op)->second; }
TokenType Op::ToOperator(string s) { return StringToOperator.find(s)->second; }
string Op::ToString(BuiltInType op) { return TypeToString.find(op)->second; }
BuiltInType Op::ToType(string s) { return StringToType.find(s)->second; }
string Op::ToString(mType* type) { return type->ToString(); }
NonTerm Op::ToType(int id) {
	if (id >= 2 && id <= 11) return NonTerm::stmt;
	if (id >= 22 && id <= 28) return NonTerm::expr;
	if (id == 12) return NonTerm::inia;
	if (id == 14 || id == 15) return NonTerm::inif;
	if (id == 16 || id == 17) return NonTerm::ini;
	if (id == 20 || id == 21) return NonTerm::exprf;
	throw(ErrDesignApp("Op::ToType(int)"));
}

bool operator==(const mVType& t1, const mVType& t2) {
	return t1.type == t2.type && t1.valuetype == t2.valuetype;
}
bool operator<=(const mVType& t1, const mVType& t2) {
	//若非（相同或左值可转换为右值），即右值不可转换为左值
	if (!(t1.valuetype == t2.valuetype || t2.valuetype == rvalue))
		return false;
	//处理隐式类型转换，允许：int->float, int literal->anything*, anything*->void*
	if (t1.type == t2.type || t1.type == TYPE(Int) && (t2.type == TYPE(Float) || t1.isLiteral && t2->isPointer()) ||
		t1->isPointer() && t2->isPointer() && t2->dereference() == TYPE(Void))
		return true;
	return false;
}

Token::Token(int lineNo) :lineNo(lineNo) {}
bool Token::isExprFollow() const { return false; }
int Token::getlineNo() const { return lineNo; }
const int* Token::getInt() const { throw(ErrDesignApp("Token::getInt")); }
const float* Token::getFloat() const { throw(ErrDesignApp("Token::getFloat")); }
const string* Token::getString() const { throw(ErrDesignApp("Token::getString")); }
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
const int* Token_Literal::getInt() const { return get_if<int>(&val); }
const float* Token_Literal::getFloat() const { return get_if<float>(&val); }
const string* Token_Literal::getString() const { return get_if<string>(&val); }

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
string Token_Operator::debug_out() const { return Op::ToString(val); }

Token_KeywordType::Token_KeywordType(int lineNo, Op::BuiltInType type) :Token_Operator(lineNo, Op::TokenType::Type), val(type) {}
Op::BuiltInType Token_KeywordType::getType() const { return val; }
string Token_KeywordType::debug_out() const { return Op::ToString(val); }

Token_End::Token_End(int lineNo) :Token(lineNo) {}
Op::TokenType Token_End::type() const { return Op::TokenType::End; }
string Token_End::debug_out() const { return ""; }

/*const Token* Op::OpLiteralCal(TokenType typ, const Token* tl, const Token* tr) {
	if (auto lstr = tl->getString(), rstr = tr->getString(); lstr && rstr) {
		//auto str = Op::LiteralCal<string>(typ, *lstr, make_optional(*rstr));
		if (lstr || rstr)
			throw(ErrDesignApp("OpLiteralCal(string, other)"));
	}
	return nullptr;
}*/

/*const mRealType AddMinusCheck(mRealType& left, mRealType& right) {
	//int	+int	=int
	//float	+float	=float
	//Point	+Point	=Point
	//ptr	+int	=ptr
	if (left->_typeid < 32 && left->_typeid == right->_typeid){
		auto t = static_cast<mTBasic*>(left->t);
		if (t == BuiltInType::Int || t == BuiltInType::Float || t == BuiltInType::Point) {
			return new mTBasic(t);
		}
	}
		

}*/

