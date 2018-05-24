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

using namespace std;

//����
struct mVar {
	mVar(mType* type, string id) :type(type), id(id) {}
	mType* type;
	string id;
};

//�﷨���ڵ�����
//�﷨�����ս������ս��Ϊ�ڵ㣬��ÿ������ʽΪ������
class GrammarTree {
public:
	virtual ~GrammarTree() {};
	virtual void changeid(int id) { throw(ErrDesignApp("GrammarTree::changeid")); }
	virtual Op::NonTerm type() { throw(ErrDesignApp("GrammarTree::type")); }
	virtual int state() { throw(ErrDesignApp("GrammarTree::state")); }
	virtual void addTree(GrammarTree* t) { throw(ErrDesignApp("GrammarTree::addTree")); }
	//������Ҫչ����vdecl
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v) { return nullptr; }
	virtual void extractlabel(map<string, GrammarTree*>& labels) {};
	//ȡ��
	virtual Token* getToken() { throw(ErrDesignApp("GrammarTree::getToken")); }
	virtual mType* getType() { throw(ErrDesignApp("GrammarTree::getType")); }
	//���ͼ�飬��������ƥ�䣬����������������goto��ǩ��
	virtual void TypeCheck() {}
};

//Ϊ��ջ��ʹ�õ�״̬���
class tState : public GrammarTree {
public:
	explicit tState(int state) :_state(state) {}
	int state() override { return _state; }
private:
	const int _state;
};

//��Ҷ
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
	template<class... Args>
	explicit tSubVars(Args&&... args) { vars.emplace_back(forward<Args>(args)...); }
	Op::NonTerm type() override { return Op::NonTerm::subv; }
	template<class... Args>
	void emplaceVar(Args&&... args) { vars.emplace_back(forward<Args>(args)...); }
private:
	vector<mVar> vars;
	friend class tSub;
};

//vdecl
class tDeclVars :public GrammarTree {
public:
	explicit tDeclVars(string id, int lineNo) :varsi({ { id, lineNo, nullptr } }), typedecl(nullptr) {}
	tDeclVars(string id, int lineNo, GrammarTree* inif) :varsi({ { id, lineNo, inif } }), typedecl(nullptr) {}
	~tDeclVars() {}
	Op::NonTerm type() override { return Op::NonTerm::vdecl; }
	void addVar(string id, int lineNo) { varsi.emplace_back(id, lineNo, nullptr); }
	void addVar(string id, int lineNo, GrammarTree* inif) { varsi.emplace_back(id, lineNo, inif); }
	void setDeclType(mType* type) { this->typedecl = move(type); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
		auto l = new list<GrammarTree*>();
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
private:
	mType* typedecl;
	vector<tuple<string, int, GrammarTree*>> varsi;	//���򴢴�
};

//����tree
class tNoVars :public GrammarTree {
public:
	explicit tNoVars(int id) :id(id) {}
	void changeid(int id) override { this->id = id; }
	template<class ... Types>
	tNoVars(int id, Types& ... args) :id(id), branchs({ args... }) {}
	Op::NonTerm type() override { return Op::ToType(id); }
	void addTree(GrammarTree* t) override { branchs.push_back(t); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
		if (id == 3)
			return branchs[0]->extractdecl(v);
		if (id >= 5 && id <= 7 || id == 9)
			branchs[0]->extractdecl(v);
		if (id == 18) {
			branchs[0]->extractdecl(v);
			branchs[1]->extractdecl(v);
		}
		return nullptr;
	}
	void extractlabel(map<string, GrammarTree*>& labels) override {
		if (id == 9) branchs[0]->extractlabel(labels); }
protected:
	int id;
	vector<GrammarTree*> branchs;	//ȫ�����򴢴棬��Ϊ����ʽ�����ҵݹ��
};

//stmts����ΪҪ����map<label, stmt*>
class tStmts :public GrammarTree {
public:
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
	list<GrammarTree*> branchs;		//ȫ�����򴢴棬��Ϊ����ʽ�����ҵݹ��
};

//����Sub����������ڹ�Լջ�����Բ�����type
class tSub : public GrammarTree {
public:
	tSub(string name, tSubVars* subv, GrammarTree* stmts) :stmts(stmts), varpara(move(subv->vars)), name(name) {
		delete subv;
		stmts->extractdecl(vardecl);
		stmts->extractlabel(labels);
	}
	void checkVar(const mVar& var) {

	}
private:
	const string name;
	const vector<mVar> varpara;			//ȫ�����򴢴�
	vector<mVar> vardecl;				//ȫ�����򴢴�
	map<string, GrammarTree*> labels;
	GrammarTree* stmts;
};

//���ڵ�
class tRoot : public GrammarTree {
public:
	Op::NonTerm type() override { return Op::NonTerm::subs; }
	void addSub(tSub* s) { subs.push_back(s); }
private:
	vector<tSub*> subs;
};