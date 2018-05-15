#include "stdafx.h"
#include "Misc.h"

const unordered_set<char> Op::OperatorChar = { '+', '-', '*', '/', '%', '&', '|', '!', '^', '=', '>', '<', '.', ';', ':', '(', ')', '{', '}', ',', '[', ']' };
const map<Op::Token, string> Op::OperatorToString = { { Plus, "+" },{ Minus, "-" },{ Times, "*" },{ Divides, "/" },{ Mod, "%" },{ EqualTo, "==" },{ NotEqual, "!=" },{ Less, "<" },{ LessEqual, "<=" },{ Greater, ">" },{ GreaterEqual, ">=" },{ Not, "!" },{ LogicalOr, "||" },{ LogicalAnd, "&&" },{ BitOr, "|" },{ BitAnd, "&" },{ BitXor, "^" },{ Negative, "(-)" },{ Deref,"(*)" },{ Address,"(&)" },{ Dot, "." },{ And, "and" },{ Or, "or" },{ Equal,"=" },{ PlusEqual,"+=" },{ MinusEqual,"-=" },{ TimesEqual,"*=" },{ DividesEqual,"/=" },{ ModEqual,"%=" },{ LogicalOrEqual,"||=" },{ LogicalAndEqual,"&&=" },{ BitOrEqual,"|=" },{ BitAndEqual,"&=" },{ BitXorEqual,"^=" },{ If, "if" },{ Else,"else" },{ Elsif,"elsif" },{ For,"for" },{ While,"while" },{ Break,"break" },{ Continue,"continue" },{ Goto,"goto" },{ Sub,"sub" },{ Semicolon,";" },{ Colon,":" },{ Bra,"(" },{ Ket,")" },{ MidBra,"[" },{ MidKet,"]" },{ BigBra,"{" },{ BigKet,"}" },{ Comma,"," } };
const map<string, Op::Token> Op::StringToOperator = swap_map(OperatorToString);
const map<Op::BuiltInType, string> TypeToString = { { Op::BuiltInType::Int,"int" },{ Op::BuiltInType::Float,"float" },{ Op::BuiltInType::Point,"point" } };
const map<string, Op::BuiltInType> Op::StringToType = swap_map(TypeToString);

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}