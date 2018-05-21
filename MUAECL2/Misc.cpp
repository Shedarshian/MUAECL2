#include "stdafx.h"
#include "Misc.h"
//#include "GrammarTree.h"

const unordered_set<char> Op::OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };
const map<Op::TokenType, string> Op::OperatorToString = { { Plus, "+" },{ Minus, "-" },{ Times, "*" },{ Divide, "/" },{ Mod, "%" },{ EqualTo, "==" },{ NotEqual, "!=" },{ Less, "<" },{ LessEqual, "<=" },{ Greater, ">" },{ GreaterEqual, ">=" },{ Not, "!" },{ LogicalOr, "||" },{ LogicalAnd, "&&" },{ BitOr, "|" },{ BitAnd, "&" },{ BitXor, "^" },{ Negative, "(-)" },{ Deref,"(*)" },{ Address,"(&)" },{ Dot, "." },{ And, "and" },{ Or, "or" },{ Equal,"=" },{ PlusEqual,"+=" },{ MinusEqual,"-=" },{ TimesEqual,"*=" },{ DividesEqual,"/=" },{ ModEqual,"%=" },{ LogicalOrEqual,"||=" },{ LogicalAndEqual,"&&=" },{ BitOrEqual,"|=" },{ BitAndEqual,"&=" },{ BitXorEqual,"^=" },{ If, "if" },{ Else,"else" },{ For,"for" },{ While,"while" },{ Break,"break" },{ Continue,"continue" },{ Goto,"goto" },{ Sub,"sub" },{ Semicolon,";" },{ Colon,":" },{ Bra,"(" },{ Ket,")" },{ MidBra,"[" },{ MidKet,"]" },{ BigBra,"{" },{ BigKet,"}" },{ Comma,"," } };
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
	case Op::LogicalOr:
		break;
	case Op::LogicalAnd:
		break;
	case Op::Or:
		break;
	case Op::And:
		break;
	case Op::BitOr:
		break;
	case Op::BitXor:
		break;
	case Op::BitAnd:
		break;
	case Op::EqualTo:
		break;
	case Op::NotEqual:
		break;
	case Op::Greater:
		break;
	case Op::GreaterEqual:
		break;
	case Op::Less:
		break;
	case Op::LessEqual:
		break;
	case Op::Plus:
		break;
	case Op::Minus:
		break;
	case Op::Times:
		break;
	case Op::Divide:
		break;
	case Op::Mod:
		break;
	case Op::Negative:
		break;
	case Op::Not:
		break;
	case Op::Deref:
		break;
	case Op::Address:
		break;
	case Op::Dot:
		break;
	case Op::MidBra:
		break;
	case Op::MidKet:
		break;
	case Op::Equal:
		break;
	case Op::PlusEqual:
		break;
	case Op::MinusEqual:
		break;
	case Op::TimesEqual:
		break;
	case Op::DividesEqual:
		break;
	case Op::ModEqual:
		break;
	case Op::LogicalOrEqual:
		break;
	case Op::LogicalAndEqual:
		break;
	case Op::BitOrEqual:
		break;
	case Op::BitAndEqual:
		break;
	case Op::BitXorEqual:
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
