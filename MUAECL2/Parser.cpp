#include "stdafx.h"
#include <algorithm>
#include <fstream>
#include "Parser.h"

using namespace std;
using Term = Op::TokenType;
using NonTerm = Op::NonTerm;

namespace Template {
	template<typename T, int last>
	struct recursiveClass {
		typedef T saveClass;
		enum { saveNum = last };
	};
	template<typename T, int ...last>
	struct unfoldClass {
		//typedef unfoldClass<typename T::saveClass, T::saveNum, last...> saveClass;
		static auto pop(stack<pair<int, GrammarTree*>> &s) { return unfoldClass<typename T::saveClass, T::saveNum, last...>::pop(s); }
	};
	template<int first, int ...count>
	struct unfoldClass<void, first, count...> {
		static auto pop(stack<pair<int, GrammarTree*>> &s) {
			//static_assert
			if constexpr (first) {
				auto t = s.top().second; s.pop();
				if constexpr (sizeof...(count) != 0)
					return tuple_cat(unfoldClass<void, count...>::pop(s), make_tuple(t));
				else
					return make_tuple(t);
			}
			else {
				delete s.top().second; s.pop();
				if constexpr (sizeof...(count) != 0)
					return unfoldClass<void, count...>::pop(s);
				else
					return tuple<>();
			}
		}
	};

	template<int first, int ...count>
	struct inverseClass {
		typedef recursiveClass<typename inverseClass<count...>::saveClass, first> saveClass;
		static auto pop(stack<pair<int, GrammarTree*>> &s) { return unfoldClass<recursiveClass<typename inverseClass<count...>::saveClass, first>>::pop(s); }
	};
	template<int first>
	struct inverseClass<first> {
		typedef recursiveClass<void, first> saveClass;
		static auto pop(stack<pair<int, GrammarTree*>> &s) { return unfoldClass<recursiveClass<void, first>>::pop(s); }
	};

	template<int ...count>
	static auto pop(stack<pair<int, GrammarTree*>> &s) {
		return inverseClass<count...>::pop(s);
	}
}

map<int, map<Op::TokenType, int>*> Parser::Action;

const map<Op::NonTerm, map<int, int>> Parser::Goto = {
	{ NonTerm::stmt, map<int, int>({ { 7, 7 }, { 9, 7 }, { 58, 60 }, { 70, 71 }, { 73, 74 }, { 75, 76 }, { 77, 7 }, { 160, 161 } }) },
	{ NonTerm::stmts, map<int, int>({ { 7, 17 }, { 9, 49 }, { 77, 78 } }) },
	{ NonTerm::subs, map<int, int>({ { 0, 255 }, { 50, 51 } }) },
	{ NonTerm::subv, map<int, int>({ { 61, 69 }, { 64, 68 } }) },
	{ NonTerm::vdecl, map<int, int>({ { 2, 12 }, { 54, 55 }, { 59, 48 } }) },
	{ NonTerm::insv, map<int, int>({ { 10, 46 }, { 43, 45 }, { 156, 157 } }) },
	{ NonTerm::ini, map<int, int>({ { 28, 29 }, { 30, 31 }, { 32, 33 }, { 53, 21 } }) },
	{ NonTerm::inif, map<int, int>({ { 53, 80 } }) },
	{ NonTerm::inia, map<int, int>({ { 20, 26 }, { 23, 25 } }) },
	{ NonTerm::exprf, map<int, int>({ { 5, 16 }, { 10, 42 }, { 43, 42 }, { 121, 122 }, { 156, 42 } }) },
	{ NonTerm::expr, map<int, int>({ { 3, 13 }, { 4, 14 }, { 5, 15 }, { 7, 87 }, { 9, 87 }, { 10, 15 }, { 20, 22 }, { 23, 22 }, { 28, 19 }, { 30, 19 }, { 32, 19 }, { 36, 37 }, { 38, 39 }, { 40, 41 }, { 43, 15 }, { 53, 19 }, { 58, 87 }, { 70, 87 }, { 73, 87 }, { 75, 87 }, { 77, 87 }, { 82, 88 }, { 83, 89 }, { 84, 92 }, { 85, 90 }, { 86, 91 }, { 101, 131 }, { 102, 132 }, { 103, 133 }, { 104, 134 }, { 105, 135 }, { 106, 136 }, { 107, 137 }, { 108, 138 }, { 109, 139 }, { 110, 140 }, { 111, 141 }, { 112, 142 }, { 113, 143 }, { 114, 144 }, { 115, 145 }, { 116, 146 }, { 117, 147 }, { 118, 148 }, { 119, 149 }, { 120, 150 }, { 121, 15 }, { 152, 153 }, { 156, 15 }, { 160, 87 }, { 163, 164 } }) },
	{ NonTerm::data, map<int, int>({ { 168, 170 }, { 176, 177 } }) },
	{ NonTerm::insdata, map<int, int>({ { 169, 173 }, { 172, 174 } }) }
};

set<int> Parser::ptr;

/*template<typename T1, typename T2, typename _Container>
inline void popd(stack<pair<T1, T2*>, _Container> &s, int n = 1) {
	for (int i = 0; i < n; i++) {
		delete s.top().second; s.pop();
	}
}*/

Parser::Parser(Tokenizer &tokenizer) :tokenizer(tokenizer), saveTree(nullptr) {}

Parser::~Parser() {
	while (!s.empty()) {
		Template::pop<0>(s);
	}
	delete saveTree;
}

void Parser::initialize() {
	ifstream f("Action.csv");
	string s;
	getline(f, s);
	while (f.good()) {
		getline(f, s, ',');
		if (s == "NonTerm")
			break;
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
}

void Parser::clear() {
	for (auto i : Action)
		if (ptr.find(i.first) == ptr.end()) {
			delete i.second;
			i.second = nullptr;
		}
		else
			i.second = nullptr;
}

tRoot* Parser::analyse() {
	try {
		s.push(make_pair(0, nullptr));
		while (1) {
			Token* t = tokenizer.peekToken();
			int n = action(s.top().first, t->type());
			if (n < 0) {
				//Error
				throw(ErrParsing(t->getlineNo(), n, t->debug_out()));
			}
			else if (n == 0) {
				//接受
				break;
			}
			else if (n < 1000) {
				//移动状态n进栈
				if constexpr (debug) clog << "Token " << t->debug_out() << " Push " << n << " to stack" << endl;
				s.push(make_pair(n, new tTerminator(t)));
				tokenizer.popToken();
			}
			else {
				//按产生式n-1000规约
				if constexpr (debug) clog << "Reading token " << t->debug_out() << " return state " << n - 1000 << endl;
				auto tree = mergeTree(n - 1000, s);
				int state = s.top().first;
				s.push(make_pair(gotostat(state, tree->type()), tree));
			}
		}
		saveTree = static_cast<tRoot*>(get<0>(Template::pop<1>(s)));
		if constexpr (debug)clog << "Parsing end\n" << endl;
		return saveTree;
	}
	catch (...) {
		//析构
		while (!s.empty()) {
			Template::pop<0>(s);
		}
		throw;
	}
}

void Parser::TypeCheck() {
	try {
		saveTree->TypeCheck(nullptr, nullptr, nullptr);
	}
	catch (...) {
		//析构
		while (!s.empty()) {
			Template::pop<0>(s);
		}
		delete saveTree;
		saveTree = nullptr;
		throw;
	}
}

fRoot Parser::Output() {
	return saveTree->Output();
}

//依据产生式id号由stack构造tree
GrammarTree* Parser::mergeTree(int id, stack<pair<int, GrammarTree*>>& s) {
	using namespace Template;
	switch (id) {
	case 12: //subs->\e
		return new tRoot();
	case 13: { //subs->sub id ( subv ) { stmts } subs
		auto[tok, subv, stmts, subs] = pop<0, 1, 0, 1, 0, 0, 1, 0, 1>(s);
		auto str = tok->getToken()->getId(); auto lineNo = tok->getToken()->getlineNo();
		delete tok;
		static_cast<tRoot*>(subs)->addSub(new tSub(str, lineNo, static_cast<tSubVars*>(subv), stmts));
		//构造函数中提取所有label与var，存到sub里
		return subs; }
	case 14: //subv->\e
		return new tSubVars();
	case 15: { //subv->type id
		auto[type, id] = pop<1, 1>(s);
		auto t = new tSubVars(type->getToken()->getType(), id->getToken()->getId());
		delete type, id;
		return t; }
	case 16: { //subv->type id, subv
		auto[type, id, subv] = pop<1, 1, 0, 1>(s);
		static_cast<tSubVars*>(subv)->emplaceVar(type->getToken()->getType(), id->getToken()->getId(), id->getToken()->getlineNo());
		delete type, id;
		return subv; }
	case 1: { //stmts->stmt stmts
		auto[stmt, stmts] = pop<1, 1>(s);
		stmts->addTree(stmt);
		return stmts; }
	case 3: //stmts->\e
		return new tStmts();
	case 4: { //stmt->expr ;
		auto[expr] = pop<1, 0>(s);
		return new tNoVars(2, expr->getLineNo(), expr); }
	case 5: { //stmt->type vdecl ;
		auto[type, vdecl] = pop<1, 1, 0>(s);
		static_cast<tDeclVars*>(vdecl)->setDeclType(type->getToken()->getType());
		delete type;
		return new tNoVars(3, -1, vdecl); }
	case 18: { //stmt->if ( expr ) stmt else stmt
		auto[expr, stmt1, stmt2] = pop<0, 0, 1, 0, 1, 0, 1>(s);
		return new tNoVars(4, -1, stmt2, stmt1, expr); }
	case 6: { //stmt->id :
		auto[id] = pop<1, 0>(s);
		auto t = new tLabel(id->getToken()->getId(), id->getToken()->getlineNo());
		delete id;
		return t; }
	case 7: //stmt->if ( expr ) stmt
		[[fallthrough]];
	case 8: //stmt->while ( expr ) stmt
		[[fallthrough]];
	case 9: { //stmt->for ( exprf ) stmt
		auto[expr, stmt] = pop<0, 0, 1, 0, 1>(s);
		return new tNoVars(id - 2, -1, stmt, expr); }
	case 10: //stmt->goto id ;
		//[0]还存指向的label对应的stmt
		[[fallthrough]];
	case 11: { //stmt->{ stmts }
		auto[t] = pop<0, 1, 0>(s);
		return new tNoVars(id - 2, -1, t); }
	case 19: //stmt->break ;
		[[fallthrough]];
	case 20: { //stmt->continue ;
		//[0]存指向的for块
		auto[t] = pop<1, 0>(s);
		auto lineNo = t->getLineNo();
		delete t;
		return new tNoVars(id - 9, lineNo); }
	case 21: { //stmt->thread id ( insv ) ;
		auto[id, insv] = pop<0, 1, 0, 1, 0, 0>(s);
		return new tNoVars(29, -1, insv, id); }
	case 22: { //stmt->do stmt while ( expr ) ;
		auto[stmt, expr] = pop<0, 1, 0, 0, 1, 0, 0>(s);
		return new tNoVars(30, -1, expr, stmt); }
	case 23: { //stmt->__rawins { data }
		auto[data] = pop<0, 0, 1, 0>(s);
		return new tNoVars(31, -1, data); }
	case 32: { //vdecl->id
		auto[id] = pop<1>(s);
		auto str = id->getToken()->getId(); auto lineNo = id->getToken()->getlineNo();
		delete id;
		return new tDeclVars(str, lineNo); }
	case 33: { //vdecl->id = inif
		auto[t1, tok, t2] = pop<1, 1, 1>(s);
		auto str = t1->getToken()->getId(); auto lineNo = t1->getToken()->getlineNo();
		auto ta = new tNoVars(2, t2->getLineNo(), new tNoVars(26, t2->getLineNo(), t2, tok, new tNoVars(22, t1->getLineNo(), t1)));
		return new tDeclVars(str, lineNo, ta); }
	case 34: { //vdecl->id , vdecl
		auto[id, vdecl] = pop<1, 0, 1>(s);
		auto str = id->getToken()->getId(); auto lineNo = id->getToken()->getlineNo();
		delete id;
		static_cast<tDeclVars*>(vdecl)->addVar(str, lineNo);
		return vdecl; }
	case 35: { //vdecl->id = inif , vdecl
		auto[id, tok, inif, vdecl] = pop<1, 1, 1, 0, 1>(s);
		auto str = id->getToken()->getId(); auto lineNo = id->getToken()->getlineNo();
		auto ta = new tNoVars(2, inif->getLineNo(), new tNoVars(26, inif->getLineNo(), inif, tok, new tNoVars(22, id->getLineNo(), id)));
		static_cast<tDeclVars*>(vdecl)->addVar(str, lineNo, ta);
		return vdecl; }
	case 29: //inia->expr
		[[fallthrough]];
	case 36: //insv->exprf
		[[fallthrough]];
	case 39: //expr->id
		[[fallthrough]];
	case 40: { //expr->num
		auto[t] = pop<1>(s);
		return new tNoVars(id - 17, t->getLineNo(), t); }
	case 28: //inia->expr , inia
		[[fallthrough]];
	case 37: { //insv->exprf , insv
		auto[t1, t2] = pop<1, 0, 1>(s);
		t2->addTree(t1);
		return t2; }
	case 38: //insv->\e
		return new tNoVars(19, -1);
	case 24: //inif->ini
		[[fallthrough]];
	case 26: //ini->expr
		[[fallthrough]];
	case 30: { //exprf->expr
		auto[t] = pop<1>(s);
		return new tNoVars(id - 10, t->getLineNo(), t); }
	case 25: //inif->ini:ini:ini:ini
		[[fallthrough]];
	case 31: { //exprf->expr:expr:expr:expr
		auto t = pop<1, 0, 1, 0, 1, 0, 1>(s);
		return new tNoVars(id - 10, get<3>(t)->getLineNo(), get<3>(t), get<2>(t), get<1>(t), get<0>(t)); }
	case 27: { //ini->{ inia }
		auto[t] = pop<0, 1, 0>(s);
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
	case 67: { //expr->expr as_op exprf
		auto[t1, tok, t2] = pop<1, 1, 1>(s);
		return new tNoVars(26, t1->getLineNo(), t2, tok, t1); }
	case 62: { //expr->( expr )
		auto[t] = pop<0, 1, 0>(s);
		return t; }
	case 59: //expr->(-) expr
		[[fallthrough]];
	case 60: //expr->! expr
		[[fallthrough]];
	case 64: //expr->(*) expr
		[[fallthrough]];
	case 65: { //expr->(&) expr
		auto[tok, t] = pop<1, 1>(s);
		return new tNoVars(25, t->getLineNo(), t, tok); }
	case 63: //expr->id ( insv )
		[[fallthrough]];
	case 66: { //expr->expr [ expr ]
		auto[t1, t2, l] = pop<1, 0, 1, 1>(s);
		//存结尾右括号的行号
		auto lineNo = l->getLineNo(); delete l;
		return new tNoVars(id - (63 - 24), lineNo, t2, t1); }
	case 68: { //expr->( type ) expr
		auto[type, t] = pop<0, 1, 0, 1>(s);
		return new tNoVars(28, t->getLineNo(), t, type); }
	case 69://data->\e
		return new tNoVars(32, -1);
	case 70: { //data->{ insdata } ; data
		auto[insdata, data] = pop<0, 1, 0, 0, 1>(s);
		data->addTree(insdata);
		return data; }
	case 71://insdata->\e
		return new tNoVars(33, -1);
	case 72: { //insdata->num insdata
		auto[num, insdata] = pop<1, 1>(s);
		insdata->addTree(num);
		return insdata; }
	default:
		return nullptr;
	}
}

int Parser::action(int s, Op::TokenType t) {
	auto m = Action.find(s)->second;
	auto it = m->find(t);
	if (it == m->end())
		throw(ErrDesignApp("Parser::action("s + to_string(s) + ", "s + Op::Ch::ToString(t)));
	return it->second;
}

int Parser::gotostat(int s, Op::NonTerm t) {
	const auto& i = Goto.find(t)->second;
	const auto& it = i.find(s);
	if (it == i.end())
		throw(ErrDesignApp("Parser::gotostat("s + to_string(s) + ", "s + to_string((int)t)));
	return it->second;
}
