#include "stdafx.h"
#include <algorithm>
#include <fstream>
#include "Parser.h"

using namespace std;
using Term = Op::TokenType;
using NonTerm = Op::NonTerm;

const map<int, map<Op::TokenType, int>*> Parser::makeAction() {
//	char sAction[] = ;
	ifstream f("Action.csv");
	map<int, map<Op::TokenType, int>*> Action;
	string s;
	getline(f, s);
	while (f.good()) {
		getline(f, s, ',');
		int index = stoi(s);
		getline(f, s, ',');
		if (s != "") {
			//如果指定ptr则直接链接，跳过这行的读取
			ptr.insert(index);
			Action[index] = Action[stoi(s)];
			getline(f, s);
		}
		else {
			//否则按顺序读取并塞入map中
			auto m = new map<Op::TokenType, int>();
			for (int i = 0; i <= (int)Term::End; i++) {
				getline(f, s, ',');
				(*m)[(Term)i] = stoi(s);
			}
			Action[index] = m;
			//抛弃剩下的占位
			getline(f, s);
		}
	}
	return Action;
}

const map<int, map<Op::TokenType, int>*> Action = Parser::makeAction();

const map<Op::NonTerm, map<int, int>> Goto = {
	{ NonTerm::stmt, map<int, int>({ { 7, 7 },{ 8, 79 },{ 9, 7 },{ 58, 60 },{ 70, 71 },{ 73, 74 },{ 75, 76 },{ 77, 7 },{ 79, 7 } }) },
	{ NonTerm::stmts, map<int, int>({ { 7, 17 },{ 9, 49 },{ 77, 78 },{ 79, 18 } }) },
	{ NonTerm::subs, map<int, int>({ { 0, 255 },{ 50, 51 } }) },
	{ NonTerm::subv, map<int, int>({ { 61, 69 } }) },
	{ NonTerm::vdecl, map<int, int>({ { 2, 12 },{ 54, 55 },{ 59, 48 } }) },
	{ NonTerm::insv, map<int, int>({ { 10, 46 },{ 43, 45 } }) },
	{ NonTerm::ini, map<int, int>({ { 28, 29 },{ 30, 31 },{ 32, 33 },{ 53, 21 } }) },
	{ NonTerm::inif, map<int, int>({ { 53, 80 } }) },
	{ NonTerm::inia, map<int, int>({ { 20, 26 },{ 23, 25 } }) },
	{ NonTerm::exprf, map<int, int>({ { 5, 16 },{ 10, 42 },{ 43, 42 },{ 121, 122 } }) },
	{ NonTerm::expr, map<int, int>({ { 3, 13 },{ 4, 14 },{ 5, 15 },{ 7, 87 },{ 8, 87 },{ 9, 87 },{ 10, 15 },{ 20, 22 },{ 23, 24 },{ 28, 19 },{ 30, 19 },{ 32, 19 },{ 36, 37 },{ 38, 39 },{ 40, 41 },{ 43, 15 },{ 53, 19 },{ 58, 87 },{ 70, 87 },{ 77, 87 },{ 82, 88 },{ 83, 89 },{ 84, 92 },{ 85, 90 },{ 86, 91 },{ 101, 131 },{ 102, 132 },{ 103, 133 },{ 104, 134 },{ 105, 135 },{ 106, 136 },{ 107, 137 },{ 108, 138 },{ 109, 139 },{ 110, 140 },{ 111, 141 },{ 112, 142 },{ 113, 143 },{ 114, 144 },{ 115, 145 },{ 116, 146 },{ 117, 147 },{ 118, 148 },{ 119, 149 },{ 120, 150 },{ 121, 15 } }) },
	{ NonTerm::types, map<int, int>({ { 9, 2 },{ 61, 62 },{ 64, 62 },{ 84, 151 } }) }
};

template<typename _Ty, typename _Container>
void popd(stack<_Ty, _Container> s, int n = 1) {
	for (int i = 0; i < n; i++) {
		delete s.top(); s.pop();
	}
}

Parser::Parser(Tokenizer &tokenizer) :tokenizer(tokenizer), saveTree(nullptr) {}

Parser::~Parser() {
	while (!s.empty()) {
		popd(s);
	}
	delete saveTree;
}

void Parser::clear() {
	for (auto i : Action)
		if (ptr.find(i.first) == ptr.end()) {
			delete i.second;
			i.second = nullptr;
		}
}

GrammarTree* Parser::analyse() {
	try {
		s.push(new tState(0));
		while (1) {
			Token* t = tokenizer.peekToken();
			int n = action(s.top()->state(), t->type());
			if (n < 0) {
				//Error
			}
			else if (n == 0) {
				//接受
				break;
			}
			else if (n < 1000) {
				//移动状态n进栈
				s.push(new tTerminator(t));
				s.push(new tState(n));
				tokenizer.popToken();
			}
			else {
				//按产生式n-1000规约
				auto t = mergeTree(n - 1000, s);
				int state = s.top()->state();
				s.push(t);
				s.push(new tState(gotostat(state, t->type())));
			}
		}
		saveTree = s.top();
		return saveTree;
	}
	catch (...) {
		//析构
		while (!s.empty()) {
			popd(s);
		}
		throw;
	}
}

void Parser::TypeCheck() {
	try {

	}
	catch (...) {
		//析构
		while (!s.empty()) {
			popd(s);
		}
		delete saveTree;
		throw;
	}
}

//依据产生式id号由stack构造tree
GrammarTree* Parser::mergeTree(int id, stack<GrammarTree*>& s) {
	switch (id) {
	case 12:
	//subs->\e
		return new tRoot();
	case 13:
	{ //subs->sub id ( subv ) { stmts } subs
		popd(s);
		auto t = s.top(); s.pop();
		popd(s, 3);
		auto stmts = s.top(); s.pop();
		popd(s, 5);
		auto subv = s.top(); s.pop();
		popd(s, 3);
		auto str = s.top()->getToken()->getId(); popd(s);
		popd(s, 2);
		static_cast<tRoot*>(t)->addSub(new tSub(str, static_cast<tSubVars*>(subv), stmts));
		//构造函数中提取所有label与var，存到sub里
		return t; }
	case 14:
	//subv->\e
		return new tSubVars();
	case 15:
	{ //subv->types id
		popd(s);
		auto str = s.top()->getToken()->getId(); popd(s);
		popd(s);
		auto typ = move(s.top()->getType().type); popd(s);
		return new tSubVars(move(typ), str); }
	case 16:
	{ //subv->types id, subv
		popd(s);
		auto t = s.top(); s.pop();
		popd(s, 3);
		auto str = s.top()->getToken()->getId(); popd(s);
		popd(s);
		auto typ = move(s.top()->getType().type); popd(s);
		static_cast<tSubVars*>(t)->emplaceVar(move(typ), str);
		return t; }
	case 1:
	{ //stmts->stmt stmts
		popd(s);
		auto t = s.top(); s.pop();
		popd(s);
		t->addTree(s.top()); s.pop();
		return t; }
	case 2:
	{ //stmts->id : stmt stmts
		popd(s);
		auto t = s.top(); s.pop();
		popd(s);
		auto t2 = s.top(); t->addTree(t2); s.pop();
		popd(s, 3);
		static_cast<tStmts*>(t)->insertlabel(
			s.top()->getToken()->getId(), t2); popd(s);
		return t; }
	case 3: //stmts->\e
		return new tStmts();
	case 4:
	{ //stmt->expr ;
		popd(s, 3);
		auto t = new tNoVars(2, s.top()); s.pop();
		return t; }
	case 5:
	{ //stmt->types vdecl ;
		popd(s, 3);
		auto t = s.top(); s.pop();
		popd(s);
		auto typ = move(s.top()->getType().type); popd(s);
		static_cast<tDeclVars*>(t)->setDeclType(move(typ));
		return new tNoVars(3, t);
	}
	case 18:
	{ //stmt->if ( expr ) stmt else stmt
		popd(s);
		auto t1 = s.top(); s.pop();
		popd(s, 3);
		auto t2 = s.top(); s.pop();
		popd(s, 3);
		auto t3 = s.top(); s.pop();
		popd(s, 4);
		return new tNoVars(4, t1, t2, t3); }
	case 7: //stmt->if ( expr ) stmt
		[[fallthrough]];
	case 8: //stmt->while ( expr ) stmt
		[[fallthrough]];
	case 9:
	{ //stmt->for ( exprf ) stmt
		popd(s);
		auto t1 = s.top(); s.pop();
		popd(s, 3);
		auto t2 = s.top(); s.pop();
		popd(s, 4);
		return new tNoVars(id - 2, t1, t2); }
	case 10: //stmt->goto id ;
		[[fallthrough]];
	case 11:
	{ //stmt->{ stmts }
		popd(s, 3);
		auto t = s.top(); s.pop();
		popd(s, 2);
		return new tNoVars(id - 2, t); }
	case 19: //stmt->break;
		[[fallthrough]];
	case 20: //stmt->continue;
		popd(s, 4);
		return new tNoVars(id - 9);
	case 21:
	{ //types->type
		popd(s);
		auto type = s.top()->getToken()->getType(); popd(s);
		return new tType(mRealType(make_unique<mTBasic>(type), Op::LRvalue::null)); }
	case 22:
	{ //types->type (*)
		popd(s, 3);
		auto t = s.top(); s.pop();
		static_cast<tType*>(t)->makePointer();
		return t; }
	case 32:
	{ //vdecl->id
		popd(s);
		auto str = s.top()->getToken()->getId(); s.pop();
		return new tDeclVars(str); }
	case 33:
	{ //vdecl->id = inif
		popd(s);
		auto t1 = s.top(); s.pop();
		popd(s);
		auto tok = s.top(); s.pop();
		popd(s);
		auto t2 = s.top(); auto str = t2->getToken()->getId(); s.pop();
		auto ta = new tNoVars(26, t1, tok, t2);
		return new tDeclVars(str, ta); }
	case 34:
	{ //vdecl->id , vdecl
		popd(s);
		auto t = s.top(); s.pop();
		popd(s, 3);
		auto str = s.top()->getToken()->getId(); s.pop();
		static_cast<tDeclVars*>(t)->addVar(str);
		return t; }
	case 35:
	{ //vdecl->id = inif , vdecl
		popd(s);
		auto t = s.top(); s.pop();
		popd(s, 3);
		auto t1 = s.top(); s.pop();
		popd(s);
		auto tok = s.top(); s.pop();
		popd(s);
		auto t2 = s.top(); auto str = t2->getToken()->getId(); s.pop();
		auto ta = new tNoVars(26, t1, tok, t2);
		static_cast<tDeclVars*>(t)->addVar(str, ta);
		return t; }
	case 29: //inia->expr
		[[fallthrough]];
	case 36: //insv->exprf
		[[fallthrough]];
	case 39: //expr->id
		[[fallthrough]];
	case 40:
	{ //expr->num
		popd(s);
		auto t = new tNoVars(id - 17, s.top()); s.pop();
		return t; }
	case 28: //inia->expr , inia
		[[fallthrough]];
	case 37:
	{ //insv->exprf , insv
		popd(s);
		auto t = s.top(); s.pop();
		popd(s, 3);
		t->addTree(s.top()); s.pop();
		return t; }
	case 38:
	//insv->\e
		return new tNoVars(19);
	case 24: //inif->ini
		[[fallthrough]];
	case 26: //ini->expr
		[[fallthrough]];
	case 30:
	{ //exprf->expr
		popd(s);
		auto t = s.top(); s.pop();
		t->changeid(id - 10);
		return t; }
	case 25:
	//inif->ini:ini:ini:ini
		[[fallthrough]];
	case 31:
	{ //exprf->expr:expr:expr:expr
		GrammarTree* t[4];
		for (int i = 0; i < 4; i++) {
			popd(s, i ? 2 : 1);
			t[i] = s.top(); s.pop();
			popd(s);
		}
		return new tNoVars(id - 10, t[0], t[1], t[2], t[3]); }
	case 27:
	{ //ini->{ inia }
		popd(s, 3);
		auto t = s.top(); s.pop();
		popd(s, 2);
		t->changeid(id - 10);
		return t; }
	case 41: //expr->expr || expr
		[[fallthrough]];
	case 42: //expr->expr && expr
		[[fallthrough]];
	case 43: //expr->expr or expr
		[[fallthrough]];
	case 44: //expr->expr and expr
		[[fallthrough]];
	case 45: //expr->expr | expr
		[[fallthrough]];
	case 46: //expr->expr ^ expr
		[[fallthrough]];
	case 47: //expr->expr & expr
		[[fallthrough]];
	case 48: //expr->expr == expr
		[[fallthrough]];
	case 49: //expr->expr != expr
		[[fallthrough]];
	case 50: //expr->expr > expr
		[[fallthrough]];
	case 51: //expr->expr >= expr
		[[fallthrough]];
	case 52: //expr->expr < expr
		[[fallthrough]];
	case 53: //expr->expr <= expr
		[[fallthrough]];
	case 54: //expr->expr + expr
		[[fallthrough]];
	case 55: //expr->expr - expr
		[[fallthrough]];
	case 56: //expr->expr * expr
		[[fallthrough]];
	case 57: //expr->expr / expr
		[[fallthrough]];
	case 58: //expr->expr % expr
		[[fallthrough]];
	case 61: //expr->expr . expr
		[[fallthrough]];
	case 67:
	{ //expr->expr as_op exprf
		popd(s);
		auto t1 = s.top(); s.pop();
		popd(s);
		auto tok = s.top(); s.pop();
		popd(s);
		auto t2 = s.top(); s.pop();
		return new tNoVars(26, t1, tok, t2); }
	case 62:
	{ //expr->( expr )
		popd(s, 3);
		auto t = s.top(); s.pop();
		popd(s, 2);
		return t; }
	case 59: //expr->(-) expr
		[[fallthrough]];
	case 60: //expr->! expr
		[[fallthrough]];
	case 64: //expr->(*) expr
		[[fallthrough]];
	case 65:
	{ //expr->(&) expr
		popd(s);
		auto t1 = s.top(); s.pop();
		popd(s);
		auto tok = s.top(); s.pop();
		return new tNoVars(25, t1, tok); }
	case 63: //expr->id ( insv )
		[[fallthrough]];
	case 66:
	{ //expr->expr [ expr ]
		popd(s, 3);
		auto t1 = s.top(); s.pop();
		popd(s, 3);
		auto t2 = s.top(); s.pop();
		return new tNoVars(id - (63 - 24) , t1, t2); }
	case 68:
	{ //expr->( types ) expr
		popd(s);
		auto t = s.top(); s.pop();
		popd(s, 3);
		auto t2 = s.top(); s.pop();
		popd(s, 2);
		return new tNoVars(28, t, t2); }
	default:
		return nullptr;
	}
}

int Parser::action(int s, Op::TokenType t) {
	auto m = Action.find(s)->second;
	auto it = m->find(t);
	if (it == m->end())
		throw("");//TODO
	return it->second;
}

int Parser::gotostat(int s, Op::NonTerm t) {
	return Goto.find(t)->second.find(s)->second;
}
