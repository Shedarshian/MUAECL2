#include "stdafx.h"
#include "GrammarTree.h"
using namespace std;

void GrammarTree::changeid(int id) { throw(ErrDesignApp("GrammarTree::changeid")); }
Op::NonTerm GrammarTree::type() { throw(ErrDesignApp("GrammarTree::type")); }
int GrammarTree::state() { throw(ErrDesignApp("GrammarTree::state")); }
void GrammarTree::addTree(GrammarTree* t) { throw(ErrDesignApp("GrammarTree::addTree")); }
list<GrammarTree*>* GrammarTree::extractdecl(vector<mVar>& v) { return nullptr; }
void GrammarTree::extractlabel(map<string, GrammarTree*>& labels) {};
Token* GrammarTree::getToken() { throw(ErrDesignApp("GrammarTree::getToken")); }
mType* GrammarTree::getType() { throw(ErrDesignApp("GrammarTree::getType")); }
int GrammarTree::getLineNo() { throw(ErrDesignApp("GrammarTree::getLineNo")); }
mType* GrammarTree::TypeCheck(tSub* sub, GrammarTree* whileBlock) { throw(ErrDesignApp("GrammarTree::TypeCheck")); }

explicit tState::tState(int state) :_state(state) {}
int tState::state() { return _state; }

explicit tTerminator::tTerminator(Token* t) :t(t) {}
tTerminator::~tTerminator() { delete t; }
Token* tTerminator::getToken() { return t; }

explicit tType::tType(Op::BuiltInType t) :t(Op::BuiltInTypeObj(t)) {}
Op::NonTerm tType::type() { return Op::NonTerm::types; }
mType* tType::getType() { return t; }
void tType::makePointer() { t = t->address(); }

tSubVars::tSubVars(mType* type, string id) { vars.emplace_back(type, id); }

void tSubVars::emplaceVar(mType* type, string id, int lineNo) {
	auto same = accumulate(vars.begin(), vars.end(), false,
		[&str = id](mVar& a, bool b) {
			return (a.id == str) || b; });
	if (same)
		throw(ErrVarRedeclared(lineNo, id));
	vars.emplace_back(type, id);
}

Op::NonTerm tSubVars::type() { return Op::NonTerm::subv; }

tDeclVars::tDeclVars(string id, int lineNo) :varsi({ { id, lineNo, nullptr } }), typedecl(nullptr) {}
tDeclVars::tDeclVars(string id, int lineNo, GrammarTree* inif) :varsi({ { id, lineNo, inif } }), typedecl(nullptr) {}
tDeclVars::~tDeclVars() { for (auto i : varsi) delete get<2>(i); }
Op::NonTerm tDeclVars::type() { return Op::NonTerm::vdecl; }
void tDeclVars::addVar(string id, int lineNo) { varsi.emplace_back(id, lineNo, nullptr); }
void tDeclVars::addVar(string id, int lineNo, GrammarTree* inif) { varsi.emplace_back(id, lineNo, inif); }
void tDeclVars::setDeclType(mType* type) { this->typedecl = type; }

list<GrammarTree*>* tDeclVars::extractdecl(vector<mVar>& v) {
	auto l = new list<GrammarTree*>();
	try {
		for (auto s : varsi) {
			auto&[var, lineNo, tree] = s;
			auto same = accumulate(varsi.begin(), varsi.end(), false,
				[&str = var](pair<string, GrammarTree*>& a, bool b) {
				return (a.first == str) || b; });
			if (same)
				throw(ErrVarRedeclared(lineNo, var));
			v.emplace_back(typedecl, var);
			if (tree != nullptr)
				l->push_back(tree);
		}
		return l;
	}
	catch (...) {
		delete l; throw;
	}
}

tNoVars::~tNoVars() {
	if (id == 8) { //stmt->goto id ;
		if (dynamic_cast<tTerminator*>(branchs[0]))
			delete branchs[0];
	}
	else if (id != 10 && id != 11) //stmt->break/continue ;
		for (auto i : branchs)
			delete i;
}

tNoVars::tNoVars(int id, int lineNo) :id(id), lineNo(lineNo) {}
void tNoVars::changeid(int id) { this->id = id; }
template<class ... Types>
tNoVars::tNoVars(int id, int lineNo, Types& ... args) : id(id), lineNo(lineNo), branchs({ args... }) {}
Op::NonTerm tNoVars::type() { return Op::ToType(id); }
void tNoVars::addTree(GrammarTree* t) { branchs.push_back(t); }

list<GrammarTree*>* tNoVars::extractdecl(vector<mVar>& v) {
	if (id == 3) {
		auto x = branchs[0]->extractdecl(v);
		delete branchs[0];
		return x;
	}
	if (id >= 5 && id <= 7 || id == 9)
		branchs[0]->extractdecl(v);
	if (id == 18) {
		branchs[0]->extractdecl(v);
		branchs[1]->extractdecl(v);
	}
	return nullptr;
}

mType* tNoVars::TypeCheck(tSub* sub, GrammarTree* whileBlock) {
	switch (id) {
	case 2:	//stmt->expr ;
		if (branchs[0]->TypeCheck(sub, whileBlock) != TYPE(Void))
			throw(ErrDiscardValue(branchs[0]->getLineNo()));
		return nullptr;
	case 3: //stmt->types vdecl ;
		throw(ErrDesignApp("GrammarTree::TypeCheck.3"));
	case 4:	//stmt->if ( expr ) stmt else stmt
		if (auto type = branchs[2]->TypeCheck(sub, whileBlock); type != TYPE(Int))
			throw ErrTypeChangeLoss(branchs[2]->getLineNo(), type, TYPE(Int));
		branchs[1]->TypeCheck(sub, whileBlock);
		return branchs[0]->TypeCheck(sub, whileBlock);
	case 5:	//stmt->if ( expr ) stmt
		[[fallthrough]];
	case 6:	//stmt->while ( expr ) stmt
		[[fallthrough]];
	case 7:	//stmt->for ( exprf ) stmt
		if (auto type = branchs[1]->TypeCheck(sub, whileBlock); type != TYPE(Int))
			throw ErrTypeChangeLoss(branchs[1]->getLineNo(), type, TYPE(Int));
		return branchs[0]->TypeCheck(sub, whileBlock);
	case 8: { //stmt->goto id ;
		auto str = branchs[0]->getToken()->getString();
		auto ptr = sub->checkLabel(*str);
		delete branchs[0];
		branchs[0] = ptr;
		return nullptr;
	}
	case 9: //stmt->{ stmts }
		return branchs[0]->TypeCheck(sub, whileBlock);
	case 10: //stmt->break ;
			 //如果没有while/for块则抛出异常
		if (whileBlock == nullptr)
			throw(ErrBreakWithoutWhile(lineNo, "break"));
		branchs.push_back(whileBlock);
		return nullptr;
	case 11: //stmt->continue ;
		if (whileBlock == nullptr)
			throw(ErrBreakWithoutWhile(lineNo, "continue"));
		branchs.push_back(whileBlock);
		return nullptr;
	case 12: //inia = vector<expr>
		throw(ErrDesignApp("GrammarTree::TypeCheck.12"));
	case 17: { //ini->{ inia } inia = vector<expr>
			   //类型检查时直接由id = inif访问ini内部寻找???

		return _type;
	}
	case 19: { //insv = vector<exprf>
			   //类型检查时直接由id(insv)访问insv内部寻找
		for (auto i : branchs)
			i->TypeCheck(sub, whileBlock);
		return nullptr;
	}
	case 14: //inif->ini
		[[fallthrough]];
	case 16: //inif->expr
		[[fallthrough]];
	case 20: //exprf->expr
		_type = branchs[0]->TypeCheck(sub, whileBlock);
		return _type;
	case 15: { //inif->ini : ini : ini : ini

	}
	case 21: { //exprf->expr : expr : expr : expr

		return _type;
	}
	case 22: { //expr->id

		return _type;
	}
	case 23: { //expr->num

		return _type;
	}
	case 24: { //expr->id ( insv )
			   //要访问insv内部
		return _type;
	}
	case 25: { //expr->Unary_op expr

		return _type;
	}
	case 26: { //expr->expr Binary_op expr/exprf/inif
		switch (branchs[1]->getToken()->type()) {
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
		case Op::Dot:
			break;
			//现在不展开赋值运算符
		case Op::Equal:
			//inif?
		case Op::PlusEqual:
			[[fallthrough]];
		case Op::MinusEqual:
			[[fallthrough]];
		case Op::TimesEqual:
			[[fallthrough]];
		case Op::DividesEqual:
			[[fallthrough]];
		case Op::ModEqual:
			[[fallthrough]];
		case Op::LogicalOrEqual:
			[[fallthrough]];
		case Op::LogicalAndEqual:
			[[fallthrough]];
		case Op::BitOrEqual:
			[[fallthrough]];
		case Op::BitAndEqual:
			[[fallthrough]];
		case Op::BitXorEqual:
			break;
		default:
			throw(ErrDesignApp("GrammarTree::TypeCheck.26"));
		}
		return _type;
	}
	case 27: { //expr->expr [ expr ]

		return _type;
	}
	case 28: { //expr->( types ) expr

			   /*mType* ExpectType(mType* expecttype, tSub* sub, GrammarTree* whileBlock) {
			   TypeCheck(sub, whileBlock);
			   if (_type != expecttype) {
			   //显式转换，int->float与float->int与ptr->ptr
			   if (_type == Op::BuiltInTypeObj(Op::BuiltInType::Int) && _type == Op::BuiltInTypeObj(Op::BuiltInType::Float)) {

			   }
			   }
			   }*/
		return _type;
	}
	default:
		throw(ErrDesignApp("GrammarTree::TypeCheck.unknown"));
	}
}

void tNoVars::extractlabel(map<string, GrammarTree*>& labels) {
	if (id == 9) branchs[0]->extractlabel(labels);
}
int tNoVars::getLineNo() { return lineNo; }

tStmts::~tStmts() { for (auto i : branchs) delete i; }
Op::NonTerm tStmts::type() { return Op::NonTerm::stmts; }
void tStmts::insertlabel(string s, int lineNo, GrammarTree* t) { labels.push_back(make_tuple(s, lineNo, t)); }
void tStmts::addTree(GrammarTree* t) { branchs.push_back(t); }

list<GrammarTree*>* tStmts::extractdecl(vector<mVar>& v) {
	for (auto it = branchs.begin(); it != branchs.end(); ++it) {
		auto t = (*it)->extractdecl(v);
		if (t != nullptr)
			move(t->begin(), t->end(), inserter(branchs, it));
		delete *it;
		branchs.erase(it);
		delete t;
	}
	return nullptr;
}

mType* tStmts::TypeCheck(tSub* sub, GrammarTree* whileBlock) {
	for (auto i : branchs)
		i->TypeCheck(sub, whileBlock);
	return nullptr;
}

void tStmts::extractlabel(map<string, GrammarTree*>& l) {
	for (auto p : labels) {
		auto&[label, lineNo, tree] = p;
		auto x = l.insert(make_pair(label, tree));
		if (!x.second)
			throw(ErrLabelRedefined(lineNo, label));
	}
}

tSub::tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts) :stmts(stmts), vardecl(subv->vars), name(name), lineNo(lineNo) {
	transform(subv->vars.begin(), subv->vars.end(), varpara.begin(), [](const mVar& t) {return t.type; });
	delete subv;
	stmts->extractdecl(vardecl);
	typeReturn = Op::BuiltInTypeObj(Op::BuiltInType::Void);		//返回值为void
	stmts->extractlabel(labels);
}

tSub::~tSub() { delete stmts; }

mType* tSub::TypeCheck(tSub* sub, GrammarTree* whileBlock) {
	stmts->TypeCheck(this, nullptr);
	return nullptr;
}

void tSub::insertDecl(map<string, vector<mType*>>& m) const {
	auto it = m.insert(make_pair(name, varpara));
	if (!it.second)
		throw(ErrFuncRedeclared(lineNo, name));
}

mVar* tSub::checkVar(string id) {
	auto itpara = find_if(vardecl.begin(), vardecl.end(), [&id](const mVar& a) { return a.id == id; });
	if (itpara == vardecl.end())
		return nullptr;
	return &(*itpara);
}

GrammarTree* tSub::checkLabel(string id) {
	auto it = labels.find(id);
	if (it == labels.end())
		return nullptr;
	return it->second;
}

int tSub::getLineNo() { return lineNo; }

tRoot::~tRoot() { for (auto i : subs) delete i; }
Op::NonTerm tRoot::type() { return Op::NonTerm::subs; }

void tRoot::addSub(tSub* s) {
	subs.push_back(s);
	s->insertDecl(subdecl);
}

mType* tRoot::TypeCheck(tSub* sub, GrammarTree* whileBlock) {
	for (auto i : subs)
		i->TypeCheck(nullptr, nullptr);
	return nullptr;
}
