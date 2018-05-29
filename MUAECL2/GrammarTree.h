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
using Op::mType;
using Op::mVType;

//����
struct mVar {
	mVar(mType* type, string id) :type(type), id(id) {}
	mType* type;
	string id;
};

class tSub;
class tRoot;
//�﷨���ڵ�����
//�﷨�����ս������ս��Ϊ�ڵ㣬��ÿ������ʽΪ������
class GrammarTree {
public:
	virtual ~GrammarTree() = default;
	virtual void changeid(int id);
	virtual Op::NonTerm type();
	virtual int state();
	virtual void addTree(GrammarTree* t);
	//������Ҫչ����vdecl
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v);
	virtual void extractlabel(map<string, GrammarTree*>& labels);
	//ȡ��
	virtual Token* getToken();
	virtual mType* getType();
	virtual int getLineNo();
	//���ͼ�飬��������ƥ�䣬����������������break������goto��ǩ��
	virtual mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock);
};

//Ϊ��ջ��ʹ�õ�״̬���
class tState : public GrammarTree {
public:
	explicit tState(int state);
	int state() override;
private:
	const int _state;
};

//��Ҷ
class tTerminator :public GrammarTree {
public:
	explicit tTerminator(Token* t);
	~tTerminator();
	Token* getToken() override;
private:
	Token* t;
};

//types
class tType : public GrammarTree {
public:
	explicit tType(Op::BuiltInType t);
	explicit tType(mType* t);
	Op::NonTerm type() override;
	mType* getType() override;
	void makePointer();
private:
	mType* t;
};

//subv
class tSubVars :public GrammarTree {
public:
	tSubVars() = default;
	tSubVars(mType* type, string id);
	void emplaceVar(mType* type, string id, int lineNo);
	Op::NonTerm type() override;
private:
	vector<mVar> vars;
	friend class tSub;
};

//vdecl
class tDeclVars :public GrammarTree {
public:
	tDeclVars(string id, int lineNo);
	tDeclVars(string id, int lineNo, GrammarTree* inif);
	~tDeclVars();
	Op::NonTerm type() override;
	void addVar(string id, int lineNo);
	void addVar(string id, int lineNo, GrammarTree* inif);
	void setDeclType(mType* type);
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
private:
	mType* typedecl;
	vector<tuple<string, int, GrammarTree*>> varsi;	//���򴢴�
};

//����tree
class tNoVars :public GrammarTree {
public:
	~tNoVars();
	tNoVars(int id, int lineNo);
	void changeid(int id) override;
	template<class ... Types>
	tNoVars(int id, int lineNo, Types&& ... args) : id(id), lineNo(lineNo), branchs({ args... }) {}
	Op::NonTerm type() override;
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& labels) override;
	int getLineNo() override;
	GrammarTree* typeChange(int rank);		//����rankֵ��Token*����
private:
	int id;
	int lineNo;
	mVType _type;
	int opID;						//���غ�������id
	vector<GrammarTree*> branchs;	//ȫ�����򴢴棬��Ϊ����ʽ�����ҵݹ��
	void LiteralCal();	//�����������Ż�
};

//stmts����ΪҪ����map<label, stmt*>
class tStmts :public GrammarTree {
public:
	~tStmts();
	Op::NonTerm type() override;
	void insertlabel(string s, int lineNo, GrammarTree* t);
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& l) override;
private:
	vector<tuple<string, int, GrammarTree*>> labels;
	list<GrammarTree*> branchs;		//ȫ�����򴢴棬��Ϊ����ʽ�����ҵݹ��
};

//����Sub����������ڹ�Լջ�����Բ�����type
class tSub : public GrammarTree {
public:
	tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts);
	~tSub();
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void insertDecl(map<string, vector<mType*>>& m) const;
	mVar* checkVar(const string& id);
	GrammarTree* checkLabel(const string& id);
	int getLineNo() override;
private:
	const string name;
	int lineNo;
	vector<mType*> varpara;				//ȫ�����򴢴�
	mType* typeReturn;					//Ŀǰ�汾��Ϊvoid���Ժ������չ��������ֵ�ĺ���
	vector<mVar> vardecl;				//ȫ�����򴢴�
	map<string, GrammarTree*> labels;
	GrammarTree* stmts;
};

//���ڵ�
class tRoot : public GrammarTree {
public:
	~tRoot();
	Op::NonTerm type() override;
	void addSub(tSub* s);
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
private:
	map<string, vector<mType*>> subdecl;
	vector<tSub*> subs;
};