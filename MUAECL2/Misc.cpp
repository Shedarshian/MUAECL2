#include "stdafx.h"
#include "Misc.h"

const unordered_set<char> Op::OperatorChar = { '+','-','*','/','%','&','|','!','^','=','>','<','.' };
const map<Operator, string> Op::OperatorToString = { { Plus, "+" },{ Minus, "-" },{ Times, "*" },{ Divides, "/" },{ Mod, "%" },{ EqualTo, "==" },{ NotEqual, "!=" },{ Less, "<" },{ LessEqual, "<=" },{ Greater, ">" },{ GreaterEqual, ">=" },{ Not, "!" },{ LogicalOr, "||" },{ LogicalAnd, "&&" },{ BitOr, "|" },{ BitAnd, "&" },{ BitXor, "^" },{ Negative, "(-)" },{ Dot, "." },{ And, "and" },{ Or, "or" },{ Sin, "sin" },{ Cos, "cos" },{ Sqrt, "sqrt" } };
const map<string, Operator> Op::StringToOperator = swap_map(OperatorToString);
const map<Assignment, string> Op::AssignmentOperatorToString = { { Equal,"=" },{ PlusEqual,"+=" },{ MinusEqual,"-=" },{ TimesEqual,"*=" },{ DividesEqual,"/=" },{ ModEqual,"%=" },{ LogicalOrEqual,"||=" },{ LogicalAndEqual,"&&=" },{ BitOrEqual,"|=" },{ BitAndEqual,"&=" },{ BitXorEqual,"^=" } };
const map<string, Assignment> Op::StringToAssignmentOperator = swap_map(AssignmentOperatorToString);
const map<Keyword, string> Op::KeywordToString = { { Int, "int" },{ Float,"float" },{ Point, "point" },{ If, "if" },{ Else,"else" },{ Elsif,"elsif" },{ For,"for" },{ While,"while" },{ Break,"break" },{ Continue,"continue" },{ Goto,"goto" },{ Sub,"sub" } };
const map<string, Keyword> Op::StringToKeyword = swap_map(KeywordToString);
const map<Operator, int> Op::Op_f = { { LogicalOr, 2 },{ LogicalAnd, 4 },{ Or, 6 },{ And, 8 },{ BitOr, 10 },{ BitXor, 12 },{ BitAnd, 14 },{ EqualTo, 16 },{ NotEqual, 16 },{ Less, 18 },{ LessEqual, 18 },{ Greater, 18 },{ GreaterEqual, 18 },{ Plus, 20 },{ Minus, 20 },{ Times, 22 },{ Divides, 22 },{ Mod, 22 },{ Negative, 22 },{ Not, 22 },{ Dot, 24 },{ Bra, 0 },{ Ket, 24 },{ End, 0 } };
const map<Operator, int> Op::Op_g = { { LogicalOr, 1 },{ LogicalAnd, 3 },{ Or, 5 },{ And, 7 },{ BitOr, 9 },{ BitXor, 11 },{ BitAnd, 13 },{ EqualTo, 15 },{ NotEqual, 15 },{ Less, 17 },{ LessEqual, 17 },{ Greater, 17 },{ GreaterEqual, 17 },{ Plus, 19 },{ Minus, 19 },{ Times, 21 },{ Divides, 21 },{ Mod, 21 },{ Negative, 25 },{ Not, 25 },{ Dot, 23 },{ Bra, 25 },{ Ket, 0 },{ End, 0 } };

const map<Op::Keyword, Token::Type> Token::KeywordToType = { { Op::Int, Token_KeywordType },{ Op::Float,Token_KeywordType },{ Op::Point,Token_KeywordType },{ Op::If,Token_KeywordIf },{ Op::Else,Token_KeywordElse },{ Op::Elsif,Token_KeywordElsif },{ Op::For,Token_KeywordFor },{ Op::While,Token_KeywordWhile },{ Op::Break,Token_KeywordBreak },{ Op::Continue,Token_KeywordContinue } ,{ Op::Goto,Token_KeywordGoto },{ Op::Sub,Token_KeywordSub } };
const map<char, Token::Type> Token::ControlChar = { { ';', Token_Semicolon },{ ':', Token_Colon },{ '(', Token_Bra },{ ')', Token_Ket },{ '{', Token_BigBra },{ '}', Token_BigKet }, { ',', Token_Comma } };
const map<Token::Type, char> Token::ControlToChar = Op::swap_map(ControlChar);

bool operator<(Operator opL, Operator opR) {
	return Op::Op_f.find(opL)->second < Op::Op_g.find(opR)->second;
}

bool operator==(Operator opL, Operator opR) {
	return Op::Op_f.find(opL)->second == Op::Op_g.find(opR)->second;
}

bool operator>(Operator opL, Operator opR) {
	return Op::Op_f.find(opL)->second > Op::Op_g.find(opR)->second;
}

bool operator<=(Operator opL, Operator opR) {
	return Op::Op_f.find(opL)->second <= Op::Op_g.find(opR)->second;
}

bool operator>=(Operator opL, Operator opR) {
	return Op::Op_f.find(opL)->second >= Op::Op_g.find(opR)->second;
}

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}