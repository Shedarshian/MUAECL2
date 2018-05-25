#pragma once
#include <array>
#include <deque>
#include <list>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <tuple>
#include <numeric>
#include "Misc.h"
#define TYPE(name) Op::BuiltInTypeObj(Op::BuiltInType::name)

using namespace std;

//变量
struct mVar {
	mVar(mType* type, string id) :type(type), id(id) {}
	mType* type;
	string id;
};

class tSub;
//语法树节点类型
//语法树以终结符与非终结符为节点，以每个产生式为子类型
class GrammarTree {
public:
	virtual ~GrammarTree() {};
	virtual void changeid(int id) { throw(ErrDesignApp("GrammarTree::changeid")); }
	virtual Op::NonTerm type() { throw(ErrDesignApp("GrammarTree::type")); }
	virtual int state() { throw(ErrDesignApp("GrammarTree::state")); }
	virtual void addTree(GrammarTree* t) { throw(ErrDesignApp("GrammarTree::addTree")); }
	//返回需要展开的vdecl
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v) { return nullptr; }
	virtual void extractlabel(map<string, GrammarTree*>& labels) {};
	//取数
	virtual Token* getToken() { throw(ErrDesignApp("GrammarTree::getToken")); }
	virtual mType* getType() { throw(ErrDesignApp("GrammarTree::getType")); }
	virtual int getLineNo() { throw(ErrDesignApp("GrammarTree::getLineNo")); }
	//类型检查，包括类型匹配，检查变量声明，处理break，计算goto标签等
	virtual mType* TypeCheck(tSub* sub, GrammarTree* whileBlock) { throw(ErrDesignApp("GrammarTree::TypeCheck")); }
};

//为入栈所使用的状态标记
class tState : public GrammarTree {
public:
	explicit tState(int state) :_state(state) {}
	int state() override { return _state; }
private:
	const int _state;
};

//树叶
class tTerminator :public GrammarTree {
public:
	explicit tTerminator(Token* t) :t(t) {}
	~tTerminator() { delete t; }
	Token* getToken() override { return t; }
private:
	Token* t;
};

//types
class tType : public GrammarTree {
public:
	explicit tType(Op::BuiltInType t) :t(Op::BuiltInTypeObj(t)) {}
	Op::NonTerm type() override { return Op::NonTerm::types; }
	mType* getType() override { return t; }
	void makePointer() { t = t->address(); }
private:
	mType* t;
};

//subv
class tSubVars :public GrammarTree {
public:
	tSubVars() {};
	//explicit tSubVars(mVar v) { vars.push_back(move(v)); }
	tSubVars(mType* type, string id) { vars.emplace_back(type, id); }
	void emplaceVar(mType* type, string id, int lineNo) {
		auto same = accumulate(vars.begin(), vars.end(), false,
			[&str = id](mVar& a, bool b) {
				return (a.id == str) || b; });
		if (same)
			throw(ErrVarRedeclared(lineNo, id));
		vars.emplace_back(type, id);
	}
	Op::NonTerm type() override { return Op::NonTerm::subv; }
private:
	vector<mVar> vars;
	friend class tSub;
};

//vdecl
class tDeclVars :public GrammarTree {
public:
	explicit tDeclVars(string id, int lineNo) :varsi({ { id, lineNo, nullptr } }), typedecl(nullptr) {}
	tDeclVars(string id, int lineNo, GrammarTree* inif) :varsi({ { id, lineNo, inif } }), typedecl(nullptr) {}
	~tDeclVars() { for (auto i : varsi) delete get<2>(i); }
	Op::NonTerm type() override { return Op::NonTerm::vdecl; }
	void addVar(string id, int lineNo) { varsi.emplace_back(id, lineNo, nullptr); }
	void addVar(string id, int lineNo, GrammarTree* inif) { varsi.emplace_back(id, lineNo, inif); }
	void setDeclType(mType* type) { this->typedecl = move(type); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
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
private:
	mType* typedecl;
	vector<tuple<string, int, GrammarTree*>> varsi;	//逆序储存
};

//其余tree
class tNoVars :public GrammarTree {
public:
	~tNoVars() {
		if (id == 8) { //stmt->goto id ;
			if (dynamic_cast<tTerminator*>(branchs[0]))
				delete branchs[0];
		}
		else if (id != 10 && id != 11) //stmt->break/continue ;
			for (auto i : branchs)
				delete i;
	}
	explicit tNoVars(int id, int lineNo) :id(id), lineNo(lineNo) {}
	void changeid(int id) override { this->id = id; }
	template<class ... Types>
	tNoVars(int id, int lineNo, Types& ... args) :id(id), lineNo(lineNo), branchs({ args... }) {}
	Op::NonTerm type() override { return Op::ToType(id); }
	void addTree(GrammarTree* t) override { branchs.push_back(t); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
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
	inline mType* TypeCheck(tSub* sub, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& labels) override {
		if (id == 9) branchs[0]->extractlabel(labels); }
	int getLineNo() override { return lineNo; }
protected:
	int id;
	int lineNo;
	mType* _type;
	vector<GrammarTree*> branchs;	//全部逆序储存，因为产生式都是右递归的
};

//stmts，因为要储存map<label, stmt*>
class tStmts :public GrammarTree {
public:
	~tStmts() { for (auto i : branchs) delete i; }
	Op::NonTerm type() override { return Op::NonTerm::stmts; }
	void insertlabel(string s, int lineNo, GrammarTree* t) { labels.push_back(make_tuple(s, lineNo, t)); }
	void addTree(GrammarTree* t) override { branchs.push_back(t); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
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
	inline mType* TypeCheck(tSub* sub, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& l) override {
		for (auto p : labels) {
			auto&[label, lineNo, tree] = p;
			auto x = l.insert(make_pair(label, tree));
			if (!x.second)
				throw(ErrLabelRedefined(lineNo, label));
		}
	}
private:
	vector<tuple<string, int, GrammarTree*>> labels;
	list<GrammarTree*> branchs;		//全部逆序储存，因为产生式都是右递归的
};

//单个Sub，不会出现在规约栈中所以不重载type
class tSub : public GrammarTree {
public:
	tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts) :stmts(stmts), vardecl(subv->vars), name(name), lineNo(lineNo) {
		transform(subv->vars.begin(), subv->vars.end(), varpara.begin(), [](const mVar& t) {return t.type; });
		delete subv;
		stmts->extractdecl(vardecl);
		typeReturn = Op::BuiltInTypeObj(Op::BuiltInType::Void);		//返回值为void
		stmts->extractlabel(labels);
	}
	~tSub() { delete stmts; }
	mType* TypeCheck(tSub* sub, GrammarTree* whileBlock) override {
		stmts->TypeCheck(this, nullptr);
		return nullptr;
	}
	void insertDecl(map<string, vector<mType*>>& m) const {
		auto it = m.insert(make_pair(name, varpara));
		if (!it.second)
			throw(ErrFuncRedeclared(lineNo, name));
	}
	mVar* checkVar(string id) {
		auto itpara = find_if(vardecl.begin(), vardecl.end(), [&id](const mVar& a) { return a.id == id; });
		if (itpara == vardecl.end())
			return nullptr;
		return &(*itpara);
	}
	GrammarTree* checkLabel(string id) {
		auto it = labels.find(id);
		if (it == labels.end())
			return nullptr;
		return it->second;
	}
	int getLineNo() override { return lineNo; }
private:
	const string name;
	int lineNo;
	vector<mType*> varpara;				//全部逆序储存
	mType* typeReturn;					//目前版本均为void，以后可能扩展至含返回值的函数
	vector<mVar> vardecl;				//全部逆序储存
	map<string, GrammarTree*> labels;
	GrammarTree* stmts;
};

//根节点
class tRoot : public GrammarTree {
public:
	~tRoot() { for (auto i : subs) delete i; }
	Op::NonTerm type() override { return Op::NonTerm::subs; }
	void addSub(tSub* s) {
		subs.push_back(s);
		s->insertDecl(subdecl);
	}
	mType* TypeCheck(tSub* sub, GrammarTree* whileBlock) override {
		for (auto i : subs)
			i->TypeCheck(nullptr, nullptr);
		return nullptr;
	}
private:
	map<string, vector<mType*>> subdecl;
	vector<tSub*> subs;
};

mType* tStmts::TypeCheck(tSub* sub, GrammarTree* whileBlock) {
	for (auto i : branchs)
		i->TypeCheck(sub, whileBlock);
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
	case 8:	{ //stmt->goto id ;
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

