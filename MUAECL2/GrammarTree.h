#pragma once
#include <array>
#include <deque>
#include <vector>
#include <utility>
#include "Misc.h"

//�﷨���ڵ�����
//�﷨�����ս������ս��Ϊ�ڵ㣬��ÿ������ʽΪ������
class GrammarTree {
public:
	GrammarTree() :id(0) {};
	GrammarTree(int id) :id(id) {};
	virtual ~GrammarTree() = 0;
	virtual Op::NonTerm getType() { /*error*/ };
	virtual int state() { /*error*/ };
	const int id;
};

//Ϊ��ջ��ʹ�õ�״̬���
class tState : public GrammarTree {
public:
	tState(int state) :_state(state) {};
	int state() override { return _state; };
private:
	const int _state;
};

//����
struct mType {
	virtual ~mType() {};
	virtual mType* clone() const = 0;
	bool operator==(mType& t2) const { return t2._typeid == _typeid; };
	virtual int _typeid() const = 0;
	//TODO ��ֵ/��ֵ
};

struct mTPointer : public mType {
	mTPointer(mType* r) :reference(r), __typeid(r->_typeid() + 32) {};
	~mTPointer() { delete reference; };
	mType* reference;
	int __typeid;
	//���ƹ��캯�����ṩ��㸴��
	mTPointer(const mTPointer& p) :mType(p._typeid) {
		reference = p.clone();
	}
	//���Ƹ�ֵ��������ṩ��㸴��
	mTPointer& operator=(const mTPointer& p) {
		delete reference;
		reference = p.clone();
		__typeid = p.__typeid;
	}
	mType* clone() const override { return new mTPointer(*this); };
	int _typeid() const override { return __typeid; };
	mType* dereference() {
		auto p = reference;
		reference = nullptr;
		delete this;
		return p;
	}
};

struct mTBasic : public mType {
	mTBasic(Op::BuiltInType t) : t(t) {};
	const Op::BuiltInType t;
	mType* clone() const override { return new mTBasic(*this); };
	int _typeid() const override { return t; };
};

//����
struct mVar {
	mVar(mType* type, string id) :type(type), id(id) {};
	~mVar() { delete type; }
	mType* type;
	string id;
};

//��Ҷ
class tTerminator :public GrammarTree {
public:
	tTerminator(Token* t) :t(t) {};
	~tTerminator() { delete t; };
private:
	Token* t;
};

/*//���ʽ
class tExpression :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::expr; };
};

//�Ѷ�switch���ʽ
class tExpression4 :public GrammarTree {
public:
	virtual ~tExpression4() = 0;
	Op::NonTerm getType() override { return Op::NonTerm::exprf; };
};

class tExpression430 :public tExpression4 {
public:
	~tExpression430() { delete expr; }
private:
	tExpression* expr;
};

class tExpression431 :public tExpression4 {
public:
	~tExpression431() { for(auto i : expr4) delete i; }
private:
	array<tExpression*, 4> expr4;
};

//initializer list
class tIni :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::ini; };
};

class tIni26 :public tIni {
public:
	~tIni26() { delete expr; }
private:
	tExpression* expr;
};

//�Ѷ�switch initializer list
class tIni4 :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::inif; };
};

class tIni424 :public tIni4 {
public:
private:
	tIni* ini;
};

class tIni425 :public tIni4 {
public:
	array<tIni*, 4> ini4;
};

//vdecl
class tVdecl :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::vdecl; };
private:
	vector<string> decl;
	vector<pair<string, tIni>> decli;
};

//���д���
class tStatement :public GrammarTree {
public:
	virtual ~tStatement() = 0;
	virtual Op::NonTerm getType() override { return Op::NonTerm::stmt; };
};

//expr ;
class tStatement4 :public tStatement {
public:
	~tStatement4() { delete expr; };
private:
	tExpression* expr;
};

//types vdecl ;
class tStatement5 :public tStatement {
public:
	~tStatement5() { delete type; delete vdecl; };
private:
	mType* type;
	tVdecl* vdecl;
};

//if ( expr ) stmt
class tStatementIf :public tStatement {
public:
	~tStatementIf() { delete expr; delete stmtif; };
private:
	tExpression* expr;
	tStatement* stmtif;
};

//if ( expr ) stmt else stmt
class tStatementIfElse :public tStatement {
public:
	~tStatementIfElse() { delete expr; delete stmtif; delete stmtelse; };
private:
	tExpression* expr;
	tStatement* stmtif;
	tStatement* stmtelse;
};

//while ( expr ) stmt
class tStatementWhile :public tStatement {
public:
	~tStatementWhile() { delete expr; delete stmt; };
private:
	tExpression* expr;
	tStatement* stmt;
};

//for ( exprf ) stmt
class tStatementFor :public tStatement {
public:
	~tStatementFor() { delete exprf; delete stmt; };
private:
	tExpression4* exprf;
	tStatement* stmt;
};

//goto id ;
class tStatementGoto :public tStatement {
private:
	string label;
};

class tStatementBreak :public tStatement {
};

class tStatementContinue :public tStatement {
};

//�����
class tBlock :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::stmts; };
};

//{ stmts }
class tStatementBlock :public tStatement {
public:
	~tStatementBlock() { delete stmts; };
private:
	tBlock* stmts;
};

//subva
class tSubva :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::subva; };
private:
	deque<mVar> vars;
};

//subv
class tSubv :public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::subv; };
private:
	deque<mVar> vars;
};*/

//��Ҫ��ֻ����var���࣬��subv,subva,vdecl,declare
class tHaveVars :public GrammarTree {
public:
	tHaveVars(int id) :GrammarTree(id) {};
private:
	vector<mVar> vars;
};

//����tree
class tNoVars :public GrammarTree {
public:
	tNoVars(int id) :GrammarTree(id) {};
private:
	vector<GrammarTree*> branchs;
};

//����Sub����������ڹ�Լջ�����Բ�����getType
class tSub : public GrammarTree {
public:
private:
	vector<mVar> vars;
	tNoVars* stmts;
};

//���ڵ�
class tRoot : public GrammarTree {
public:
	Op::NonTerm getType() override { return Op::NonTerm::subs; };
private:
	vector<tSub*> subs;
};