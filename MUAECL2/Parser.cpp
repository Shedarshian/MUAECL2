#include "stdafx.h"
#include "Parser.h"
#include <algorithm>

using namespace std;
using Term = Op::Token;
using NonTerm = Parser::NonTerminator;

const map<Parser::NonTerminator, map<int, int>> Goto = {
	{ NonTerm::stmt, map<int, int>({ { 7, 7 },{ 8, 79 },{ 9, 7 },{ 58, 60 },{ 70, 71 },{ 73, 74 },{ 75, 76 },{ 77, 7 },{ 79, 7 } }) },
	{ NonTerm::stmts, map<int, int>({ { 7, 17 },{ 9, 49 },{ 77, 78 },{ 79, 18 } }) },
	{ NonTerm::subs, map<int, int>({ { 0, 255 },{ 50, 51 } }) },
	{ NonTerm::subv, map<int, int>({ { 61, 69 } }) },
	{ NonTerm::subva, map<int, int>({ { 63, 67 },{ 66, 68 } }) },
	{ NonTerm::vdecl, map<int, int>({ { 2, 12 },{ 54, 55 },{ 59, 48 } }) },
	{ NonTerm::insv, map<int, int>({ { 10, 46 },{ 43, 45 } }) },
	{ NonTerm::ini, map<int, int>({ { 28, 29 },{ 30, 31 },{ 32, 33 },{ 53, 21 } }) },
	{ NonTerm::inif, map<int, int>({ { 53, 80 } }) },
	{ NonTerm::inia, map<int, int>({ { 22, 26 },{ 24, 25 } }) },
	{ NonTerm::exprf, map<int, int>({ { 5, 16 },{ 10, 42 },{ 43, 42 },{ 121, 122 } }) },
	{ NonTerm::expr, map<int, int>({ { 3, 13 },{ 4, 14 },{ 5, 15 },{ 7, 87 },{ 8, 87 },{ 9, 87 },{ 10, 15 },{ 20, 22 },{ 23, 24 },{ 28, 19 },{ 30, 19 },{ 32, 19 },{ 36, 37 },{ 38, 39 },{ 40, 41 },{ 43, 15 },{ 53, 19 },{ 58, 87 },{ 70, 87 },{ 77, 87 },{ 82, 88 },{ 83, 89 },{ 84, 92 },{ 85, 90 },{ 86, 91 },{ 101, 131 },{ 102, 132 },{ 103, 133 },{ 104, 134 },{ 105, 135 },{ 106, 136 },{ 107, 137 },{ 108, 138 },{ 109, 139 },{ 110, 140 },{ 111, 141 },{ 112, 142 },{ 113, 143 },{ 114, 144 },{ 115, 145 },{ 116, 146 },{ 117, 147 },{ 118, 148 },{ 119, 149 },{ 120, 150 },{ 121, 15 } }) },
	{ NonTerm::types, map<int, int>({ { 9, 2 },{ 61, 62 },{ 64, 65 },{ 84, 151 } }) }
};

Parser::Parser(Tokenizer &tokenizer) :tokenizer(tokenizer) {}

Parser::~Parser() {}

void Parser::initialize(ifstream& f) {
	string s;
	//抛弃索引行
	getline(f, s);
	while (!f.eof()) {
		getline(f, s, ',');
		if (s == "NonTerm")
			break;
		int index = stoi(s);
		getline(f, s, ',');
		if (s != "") {
			//如果指定ptr则直接链接，跳过这行的读取
			Action[index] = Action[stoi(s)];
			getline(f, s);
		}
		else {
			//否则按顺序读取并塞入map中
			auto m = new map<Op::Token, int>();
			for (int i = 0; i <= Term::End; i++) {
				getline(f, s, ',');
				(*m)[(Term)i] = stoi(s);
			}
			Action[index] = m;
			//抛弃剩下的占位
			getline(f, s);
		}
	}
	//goto表手动塞好了
	/* //抛弃索引行
	getline(f, s);
	while (!f.eof()) {
		getline(f, s, ',');
		int index = stoi(s);
		getline(f, s, ',');
		for (int i = 0; i <= NonTerm::types; i++) {
			getline(f, s, ',');
			if (s != "")
				Goto[(NonTerm)i][index] = stoi(s);
		}
	}*/
}

int Parser::action(int s, Op::Token t) {
	auto m = Action.find(s)->second;
	auto it = m->find(t);
	if (it == m->end())
		throw("");//TODO
	return it->second;
}

int Parser::gotostat(int s, Parser::NonTerminator t) {
	return Goto.find(t)->second.find(s)->second;
}
