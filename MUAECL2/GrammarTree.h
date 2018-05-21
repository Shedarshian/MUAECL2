#pragma once
#include <array>
#include <deque>
#include <list>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include "Misc.h"

using namespace std;

//����
struct mVar {
	mVar(unique_ptr<mType> type, string id) :type(move(type)), id(id) {}
	~mVar() {}
	unique_ptr<mType> type;
	string id;
};

//�﷨���ڵ�����
//�﷨�����ս������ս��Ϊ�ڵ㣬��ÿ������ʽΪ������
class GrammarTree {
public:
	virtual ~GrammarTree() = 0;
	virtual void changeid(int id) { throw(ErrDesignApp("GrammarTree::changeid")); }
	virtual Op::NonTerm type() { throw(ErrDesignApp("GrammarTree::type")); }
	virtual int state() { throw(ErrDesignApp("GrammarTree::state")); }
	virtual void addTree(GrammarTree* t) { throw(ErrDesignApp("GrammarTree::addTree")); }
	//������Ҫչ����vdecl
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v) { return nullptr; }
	virtual void extractlabel(map<string, GrammarTree*>& labels) {};
	//ȡ��
	virtual Token* getToken() { throw(ErrDesignApp("GrammarTree::getToken")); }
	virtual mRealType& getType() { throw(ErrDesignApp("GrammarTree::getType")); }
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
	explicit tType(mRealType t) :t(move(t)) {}
	~tType() {}
	Op::NonTerm type() override { return Op::NonTerm::types; }
	mRealType& getType() override { return t; }
	void makePointer() { t.makePointer(); }
private:
	mRealType t;
};

//subv
class tSubVars :public GrammarTree {
public:
	tSubVars() = default;
	//explicit tSubVars(mVar v) { vars.push_back(move(v)); }
	template<class... Args>
	explicit tSubVars(Args&&... args) { vars.emplace_back(&args...); }
	Op::NonTerm type() override { return Op::NonTerm::subv; }
	template<class... Args>
	void emplaceVar(Args&&... args) { vars.emplace_back(&args...); }
private:
	vector<mVar> vars;
	friend class tSub;
};

//vdecl
class tDeclVars :public GrammarTree {
public:
	explicit tDeclVars(string id) :varsi({ { id, nullptr } }), typedecl(nullptr) {}
	tDeclVars(string id, GrammarTree* inif) :varsi({ { id, inif } }), typedecl(nullptr) {}
	~tDeclVars() {}
	Op::NonTerm type() override { return Op::NonTerm::vdecl; }
	void addVar(string id) { varsi.emplace_back(id, nullptr); }
	void addVar(string id, GrammarTree* inif) { varsi.emplace_back(id, inif); }
	void setDeclType(unique_ptr<mType> type) { this->typedecl = move(type); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
		auto l = new list<GrammarTree*>();
		for (auto s : varsi) {
			v.emplace_back(typedecl->clone(), s.first);
			if (s.second != nullptr)
				l->push_back(s.second);
		}
		return l;
	}
private:
	unique_ptr<mType> typedecl;
	vector<pair<string, GrammarTree*>> varsi;	//���򴢴�
};

//����tree
class tNoVars :public GrammarTree {
public:
	explicit tNoVars(int id) :id(id) {}
	void changeid(int id) override { this->id = id; }
	template<class ... Types>
	tNoVars(int id, Types ... args) :GrammarTree(id), branchs({ &args... }) {}
	Op::NonTerm type() override { return Op::ToType(id); }
	void addTree(GrammarTree* t) override { branchs.push_back(t); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
		if (id == 3) {
			return branchs[0]->extractdecl(v);
		}
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
	void insertlabel(string s, GrammarTree* t) { labels.push_back(make_pair(s, t)); }
	void addTree(GrammarTree* t) override { branchs.push_back(t); }
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override {
		for (auto it = branchs.begin(); it != branchs.end(); ++it) {
			auto t = (*it)->extractdecl(v);
			if (t != nullptr)
				move(t->begin(), t->end(), insert_iterator<decltype(branchs)>(branchs, it));
			delete *it;
			branchs.erase(it);
			delete t;
		}
		return nullptr;
	}
	void extractlabel(map<string, GrammarTree*>& l) override {
		move(labels.begin(), labels.end(), insert_iterator<map<string, GrammarTree*>>(l, l.begin()));
	}
private:
	vector<pair<string, GrammarTree*>> labels;
	list<GrammarTree*> branchs;		//ȫ�����򴢴棬��Ϊ����ʽ�����ҵݹ��
};

//����Sub����������ڹ�Լջ�����Բ�����type
class tSub : public GrammarTree {
public:
	tSub(string name, tSubVars* subv, GrammarTree* stmts) :stmts(stmts), varpara(move(subv->vars)), name(name) {
		delete subv;
		stmts->extractdecl(vardecl);
		stmts->extractlabel(labels);
		//TODO label
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