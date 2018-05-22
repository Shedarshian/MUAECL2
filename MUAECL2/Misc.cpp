#include "stdafx.h"
#include "Misc.h"
//#include "GrammarTree.h"
using TT = Op::TokenType;

const unordered_set<char> Op::OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };
const map<TT, string> Op::OperatorToString = { { TT::Plus, "+" },{ TT::Minus, "-" },{ TT::Times, "*" },{ TT::Divide, "/" },{ TT::Mod, "%" },{ TT::EqualTo, "==" },{ TT::NotEqual, "!=" },{ TT::Less, "<" },{ TT::LessEqual, "<=" },{ TT::Greater, ">" },{ TT::GreaterEqual, ">=" },{ TT::Not, "!" },{ TT::LogicalOr, "||" },{ TT::LogicalAnd, "&&" },{ TT::BitOr, "|" },{ TT::BitAnd, "&" },{ TT::BitXor, "^" },{ TT::Negative, "(-)" },{ TT::Deref,"(*)" },{ TT::Address,"(&)" },{ TT::Dot, "." },{ TT::And, "and" },{ TT::Or, "or" },{ TT::Equal,"=" },{ TT::PlusEqual,"+=" },{ TT::MinusEqual,"-=" },{ TT::TimesEqual,"*=" },{ TT::DividesEqual,"/=" },{ TT::ModEqual,"%=" },{ TT::LogicalOrEqual,"||=" },{ TT::LogicalAndEqual,"&&=" },{ TT::BitOrEqual,"|=" },{ TT::BitAndEqual,"&=" },{ TT::BitXorEqual,"^=" },{ TT::If, "if" },{ TT::Else,"else" },{ TT::For,"for" },{ TT::While,"while" },{ TT::Break,"break" },{ TT::Continue,"continue" },{ TT::Goto,"goto" },{ TT::Sub,"sub" },{ TT::Semicolon,";" },{ TT::Colon,":" },{ TT::Bra,"(" },{ TT::Ket,")" },{ TT::MidBra,"[" },{ TT::MidKet,"]" },{ TT::BigBra,"{" },{ TT::BigKet,"}" },{ TT::Comma,"," } };
const map<string, Op::TokenType> Op::StringToOperator = swap_map(OperatorToString);
const map<Op::BuiltInType, string> TypeToString = { { Op::BuiltInType::Int,"int" },{ Op::BuiltInType::Float,"float" },{ Op::BuiltInType::Point,"point" } };
const map<string, Op::BuiltInType> Op::StringToType = swap_map(TypeToString);

/*const mRealType AddMinusCheck(mRealType& left, mRealType& right) {
	//int	+int	=int
	//float	+float	=float
	//Point	+Point	=Point
	//ptr	+int	=ptr
	if (left->_typeid < 32 && left->_typeid == right->_typeid){
		auto t = static_cast<mTBasic*>(left->t);
		if (t == Op::BuiltInType::Int || t == Op::BuiltInType::Float || t == Op::BuiltInType::Point) {
			return new mTBasic(t);
		}
	}
		

}*/

const Token* Op::OpLiteralCal(TokenType typ, const Token* tl, const Token* tr) {
	switch (typ) {
	case Op::TokenType::LogicalOr:
		break;
	case Op::TokenType::LogicalAnd:
		break;
	case Op::TokenType::Or:
		break;
	case Op::TokenType::And:
		break;
	case Op::TokenType::BitOr:
		break;
	case Op::TokenType::BitXor:
		break;
	case Op::TokenType::BitAnd:
		break;
	case Op::TokenType::EqualTo:
		break;
	case Op::TokenType::NotEqual:
		break;
	case Op::TokenType::Greater:
		break;
	case Op::TokenType::GreaterEqual:
		break;
	case Op::TokenType::Less:
		break;
	case Op::TokenType::LessEqual:
		break;
	case Op::TokenType::Plus:
		break;
	case Op::TokenType::Minus:
		break;
	case Op::TokenType::Times:
		break;
	case Op::TokenType::Divide:
		break;
	case Op::TokenType::Mod:
		break;
	case Op::TokenType::Negative:
		break;
	case Op::TokenType::Not:
		break;
	case Op::TokenType::Deref:
		break;
	case Op::TokenType::Address:
		break;
	case Op::TokenType::Dot:
		break;
	case Op::TokenType::MidBra:
		break;
	case Op::TokenType::MidKet:
		break;
	case Op::TokenType::Equal:
		break;
	case Op::TokenType::PlusEqual:
		break;
	case Op::TokenType::MinusEqual:
		break;
	case Op::TokenType::TimesEqual:
		break;
	case Op::TokenType::DividesEqual:
		break;
	case Op::TokenType::ModEqual:
		break;
	case Op::TokenType::LogicalOrEqual:
		break;
	case Op::TokenType::LogicalAndEqual:
		break;
	case Op::TokenType::BitOrEqual:
		break;
	case Op::TokenType::BitAndEqual:
		break;
	case Op::TokenType::BitXorEqual:
		break;
	default:
		//throw
	}
	return nullptr;
}

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}
