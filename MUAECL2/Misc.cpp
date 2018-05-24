#include "stdafx.h"
#include "Misc.h"
//#include "GrammarTree.h"
using namespace Op;

mType* mType::address() {
	if (_address == nullptr)
		_address = new mTypePointer(this);
	return _address;
}

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

const Token* OpLiteralCal(TokenType typ, const Token* tl, const Token* tr) {
	/*switch (typ) {
	case TokenType::LogicalOr:
		break;
	case TokenType::LogicalAnd:
		break;
	case TokenType::Or:
		break;
	case TokenType::And:
		break;
	case TokenType::BitOr:
		break;
	case TokenType::BitXor:
		break;
	case TokenType::BitAnd:
		break;
	case TokenType::EqualTo:
		break;
	case TokenType::NotEqual:
		break;
	case TokenType::Greater:
		break;
	case TokenType::GreaterEqual:
		break;
	case TokenType::Less:
		break;
	case TokenType::LessEqual:
		break;
	case TokenType::Plus:
		break;
	case TokenType::Minus:
		break;
	case TokenType::Times:
		break;
	case TokenType::Divide:
		break;
	case TokenType::Mod:
		break;
	case TokenType::Negative:
		break;
	case TokenType::Not:
		break;
	case TokenType::Deref:
		break;
	case TokenType::Address:
		break;
	case TokenType::Dot:
		break;
	case TokenType::MidBra:
		break;
	case TokenType::MidKet:
		break;
	case TokenType::Equal:
		break;
	case TokenType::PlusEqual:
		break;
	case TokenType::MinusEqual:
		break;
	case TokenType::TimesEqual:
		break;
	case TokenType::DividesEqual:
		break;
	case TokenType::ModEqual:
		break;
	case TokenType::LogicalOrEqual:
		break;
	case TokenType::LogicalAndEqual:
		break;
	case TokenType::BitOrEqual:
		break;
	case TokenType::BitAndEqual:
		break;
	case TokenType::BitXorEqual:
		break;
	default:
		//throw
		break;
	}*/
	return nullptr;
}

template<typename T>
const T& LiteralCal(TokenType typ, const T& t1, const optional<T>& t2) {
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

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}
