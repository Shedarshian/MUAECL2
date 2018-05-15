#include "stdafx.h"
#include "Parser.h"
#include <algorithm>

using namespace std;
using Term = Parser::Terminator;
using NonTerm = Parser::NonTerminator;

//复制一个map并将其value加i
auto ma(map<Term, int>* s, int i)->decltype(s) {
	decltype(s) t = new map<Term, int>();
	for (auto it : *s)
		(*t)[it.first] = it.second + i;
	return t;
}

map<int, map<Parser::Terminator, int>*> makeAction() {
	map<int, map<Parser::Terminator, int>*> a;
	a[127] = new map<Term, int>({ { Term::bigket, -1 },{ Term::end, -1 } });
	a[0] = new map<Term, int>({ { Term::identifier, 1 },{ Term::bigbra, 77 },{ Term::bigket, 1003 },{ Term::end, 1003 },{ Term::type, 2 },{ Term::if_bra, 3 },{ Term::while_bra, 4 },{ Term::for_bra, 5 },{ Term::goto_id_colon, 6 } });
	a[1] = new map<Term, int>({ { Term::colon, 8 },{ Term::bra, 10 },{ Term::equal, 9 },{ Term::assignment_operator, 9 } });
	a[2] = new map<Term, int>({ { Term::identifier, 11 } });
	a[3] = new map<Term, int>({ { Term::expression, 13 } });
	a[4] = ma(a[4], 1);
	a[5] = ma(a[5], 1);
	a[6] = new map<Term, int>({ { Term::identifier, 1010 },{ Term::bigbra, 1010 },{ Term::bigket, 1010 },{ Term::end, 1010 },{ Term::type, 1010 },{ Term::if_bra, 1010 },{ Term::else_bra, 1010 },{ Term::while_bra, 1010 },{ Term::for_bra, 1010 },{ Term::goto_id_colon, 1010 } });
	a[7] = a[0];
	a[8] = new map<Term, int>({ { Term::identifier, 59 },{ Term::bigbra, 77 },{ Term::type, 2 },{ Term::if_bra, 3 },{ Term::while_bra, 4 },{ Term::for_bra, 5 },{ Term::goto_id_colon, 6 } });
	a[9] = new map<Term, int>({ { Term::bigbra, 20 },{ Term::expression, 19 } });
	a[10] = new map<Term, int>({ { Term::ket, 1018 },{ Term::expression, 15 } });
	a[11] = new map<Term, int>({ { Term::semicolon, 1032 },{ Term::comma, 60 },{ Term::equal, 53 } });
	a[12] = new map<Term, int>({ { Term::comma, 57 } });
	a[13] = new map<Term, int>({ { Term::ket, 58 } });
	a[14] = new map<Term, int>({ { Term::ket, 73 } });
	a[15] = new map<Term, int>({ { Term::colon, 36 },{ Term::comma, 1030 },{ Term::ket, 1030 } });
	a[16] = new map<Term, int>({ { Term::ket, 75 } });
	a[17] = new map<Term, int>({ { Term::bigket, 1001 },{ Term::end, 1001 } });
	a[18] = ma(a[17], 1);
	a[19] = new map<Term, int>({ { Term::colon, 1026 },{ Term::semicolon, 1026 },{ Term::comma, 1026 } });
	a[20] = new map<Term, int>({ { Term::expression, 22 } });
	a[21] = new map<Term, int>({ { Term::colon, 28 },{ Term::semicolon, 1024 },{ Term::comma, 1024 } });
	a[22] = new map<Term, int>({ { Term::comma, 23 },{ Term::bigket, 1029 } });
	a[23] = new map<Term, int>({ { Term::expression, 24} });
	a[24] = a[22];
	a[25] = new map<Term, int>({ { Term::bigket, 1028 } });
	a[26] = new map<Term, int>({ { Term::bigket, 27 } });
	a[27] = ma(a[19], 1);
	a[28] = a[9];
	a[29] = new map<Term, int>({ { Term::colon, 30 } });
	a[30] = a[9];
	a[31] = new map<Term, int>({ { Term::colon, 32 } });
	a[32] = a[9];
	a[33] = new map<Term, int>({ { Term::semicolon, 1025 },{ Term::comma, 1025 } });
	a[34] = ma(a[6], -6);
	a[36] = new map<Term, int>({ { Term::expression, 37 } });
	a[37] = new map<Term, int>({ {Term::colon, 38} });
	a[38] = ma(a[36], 2);
	a[39] = ma(a[37], 2);
	a[40] = ma(a[38], 2);
	a[41] = new map<Term, int>({ { Term::comma, 1031 },{ Term::ket, 1031 } });
	a[42] = new map<Term, int>({ { Term::comma, 43 },{ Term::ket, 1036 } });
	a[43] = new map<Term, int>({ { Term::ket, 1018 },{ Term::expression, 15 } });
	a[45] = new map<Term, int>({ { Term::ket, 1037 } });
	a[46] = new map<Term, int>({ { Term::ket, 61 } });
	a[47] = ma(a[6], -4);
	a[48] = new map<Term, int>({ { Term::semicolon, 1034 } });
	a[53] = a[9];
	a[54] = a[2];
	a[55] = ma(a[48], 1);
	a[57] = ma(a[6], -5);
	a[58] = a[8];
	a[59] = new map<Term, int>({ { Term::bra, 10 },{ Term::equal, 9 },{ Term::assignment_operator, 9 } });
	a[60] = a[2];
	a[61] = new map<Term, int>({ { Term::semicolon, 47 } });
	a[62] = new map<Term, int>({ { Term::identifier, 48 } });
	a[63] = a[62];
	a[64] = ma(a[6], 13); (*a[64])[Term::else_bra] = 70;
	a[70] = a[8];
	a[71] = ma(a[6], 12);
	a[72] = ma(a[6], -3);
	a[73] = a[8];
	a[74] = ma(a[6], -2);
	a[75] = a[8];
	a[76] = ma(a[6], -1);
	a[77] = a[8];
	a[78] = new map<Term, int>({ { Term::bigket, 81 } });
	a[79] = a[0];
	a[80] = new map<Term, int>({ { Term::semicolon, 1033 },{ Term::comma, 54 } });
	a[81] = ma(a[6], 1);
	return a;
}

const map<int, map<Parser::Terminator, int>*> Action = makeAction();
const map<Parser::NonTerminator, map<int, int>> Goto = {
	{ NonTerm::stmt, map<int, int>({ { 0, 7 },{ 7, 7 },{ 8, 79 },{ 58, 64 },{ 70, 71 },{ 73, 74 },{ 75, 76 },{ 77, 7 },{ 79, 7 } }) },
	{ NonTerm::stmts, map<int, int>({ { 0, 127 },{ 7, 17 },{ 77, 78 },{ 79, 18 } }) },
	{ NonTerm::vdecl, map<int, int>({ { 2, 12 },{ 54, 55 },{ 60, 48 } }) },
	{ NonTerm::insv, map<int, int>({ { 10, 46 },{ 43, 45 } }) },
	{ NonTerm::ifa, map<int, int>({ { 64, 72 } }) },
	{ NonTerm::ini, map<int, int>({ { 9, 21 },{ 28, 29 },{ 30, 31 },{ 32, 33 },{ 53, 21 } }) },
	{ NonTerm::inif, map<int, int>({ { 9, 34 },{ 53, 80 } }) },
	{ NonTerm::inia, map<int, int>({ { 22, 26 },{ 24, 25 } }) },
	{ NonTerm::exprf, map<int, int>({ { 5, 16 },{ 10, 42 },{ 43, 42 } }) }
};

Parser::Parser(Tokenizer &tokenizer) :tokenizer(tokenizer) {}

Parser::~Parser() {}

int Parser::action(int s, Terminator t) {
	auto m = Action.find(s)->second;
	auto it = m->find(t);
	if (it == m->end())
		throw("");//TODO
	return it->second;
}

int Parser::gotostat(int s, NonTerminator t) {
	return Goto.find(t)->second.find(s)->second;
}
