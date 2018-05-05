#include "stdafx.h"
#include "Misc.h"

const unordered_set<char> Op::OperatorChar = { '+','-','*','/','%','&','|','!','^','=','>','<','.' };
const map<Operator, string> Op::OperatorToString = { { Plus, "+" },{ Minus, "-" },{ Times, "*" },{ Divides, "/" },{ Mod, "%" },{ EqualTo, "==" },{ NotEqual, "!=" },{ Less, "<" },{ LessEqual, "<=" },{ Greater, ">" },{ GreaterEqual, ">=" },{ Not, "!" },{ LogicalOr, "||" },{ LogicalAnd, "&&" },{ BitOr, "|" },{ BitAnd, "&" },{ BitXor, "^" },{ Negative, "(-)" },{ Dot, "." },{ And, "and" },{ Or, "or" } };
const map<string, Operator> Op::StringToOperator = swap_map(OperatorToString);
const map<Assignment, string> Op::AssignmentOperatorToString = { { Equal,"=" },{ PlusEqual,"+=" },{ MinusEqual,"-=" },{ TimesEqual,"*=" },{ DividesEqual,"/=" },{ ModEqual,"%=" },{ LogicalOrEqual,"||=" },{ LogicalAndEqual,"&&=" },{ BitOrEqual,"|=" },{ BitAndEqual,"&=" },{ BitXorEqual,"^=" } };
const map<string, Assignment> Op::StringToAssignmentOperator = swap_map(AssignmentOperatorToString);
const map<Keyword, string> Op::KeywordToString = { { Int, "int" },{ Float,"float" },{ Point, "point" },{ If, "if" },{ Else,"else" },{ Elsif,"elsif" },{ For,"for" },{ While,"while" },{ Break,"break" },{ Continue,"continue" },{ Goto,"goto" },{ Sub,"sub" } };
const map<string, Keyword> Op::StringToKeyword = swap_map(KeywordToString);

const map<Op::Keyword, Token::Type> Token::KeywordToType = { { Op::Int, Token_KeywordType },{ Op::Float,Token_KeywordType },{ Op::Point,Token_KeywordType },{ Op::If,Token_KeywordIf },{ Op::Else,Token_KeywordElse },{ Op::Elsif,Token_KeywordElsif },{ Op::For,Token_KeywordFor },{ Op::While,Token_KeywordWhile },{ Op::Break,Token_KeywordBreak },{ Op::Continue,Token_KeywordContinue } ,{ Op::Goto,Token_KeywordGoto },{ Op::Sub,Token_KeywordSub } };
const map<char, Token::Type> Token::ControlChar = { { ';', Token_Semicolon },{ ':', Token_Colon },{ '(', Token_Bra },{ ')', Token_Ket },{ '{', Token_BigBra },{ '}', Token_BigKet } };
const map<Token::Type, char> Token::ControlToChar = Op::swap_map(ControlChar);

ostream& operator<< (ostream& stream, Token& token) {
	stream << token.debug_out();
	return stream;
}