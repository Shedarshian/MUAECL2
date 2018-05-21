#include "stdafx.h"
#include "Misc.h"
//#include "GrammarTree.h"

const unordered_set<char> Op::OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };
const map<Op::Token, string> Op::OperatorToString = { { Plus, "+" },{ Minus, "-" },{ Times, "*" },{ Divide, "/" },{ Mod, "%" },{ EqualTo, "==" },{ NotEqual, "!=" },{ Less, "<" },{ LessEqual, "<=" },{ Greater, ">" },{ GreaterEqual, ">=" },{ Not, "!" },{ LogicalOr, "||" },{ LogicalAnd, "&&" },{ BitOr, "|" },{ BitAnd, "&" },{ BitXor, "^" },{ Negative, "(-)" },{ Deref,"(*)" },{ Address,"(&)" },{ Dot, "." },{ And, "and" },{ Or, "or" },{ Equal,"=" },{ PlusEqual,"+=" },{ MinusEqual,"-=" },{ TimesEqual,"*=" },{ DividesEqual,"/=" },{ ModEqual,"%=" },{ LogicalOrEqual,"||=" },{ LogicalAndEqual,"&&=" },{ BitOrEqual,"|=" },{ BitAndEqual,"&=" },{ BitXorEqual,"^=" },{ If, "if" },{ Else,"else" },{ For,"for" },{ While,"while" },{ Break,"break" },{ Continue,"continue" },{ Goto,"goto" },{ Sub,"sub" },{ Semicolon,";" },{ Colon,":" },{ Bra,"(" },{ Ket,")" },{ MidBra,"[" },{ MidKet,"]" },{ BigBra,"{" },{ BigKet,"}" },{ Comma,"," } };
const map<string, Op::Token> Op::StringToOperator = swap_map(OperatorToString);
const map<Op::BuiltInType, string> TypeToString = { { Op::BuiltInType::Int,"int" },{ Op::BuiltInType::Float,"float" },{ Op::BuiltInType::Point,"point" } };
const map<string, Op::BuiltInType> Op::StringToType = swap_map(TypeToString);

const mType* AddMinusCheck(mType* left, mType* right) {
	//int	+int	=int
	//float	+float	=float
	//Point	+Point	=Point
	//ptr	+int	=ptr
	if (left->_typeid < 32 && left->_typeid == right->_typeid){
		auto t = static_cast<mTBasic*>(left)->t;
		if (t == Op::BuiltInType::Int || t == Op::BuiltInType::Float || t == Op::BuiltInType::Point) {
			delete left; delete right;
			return new mTBasic(t);
		}
	}
		

}

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}