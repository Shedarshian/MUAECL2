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
#include "Misc2.h"

using namespace std;
using Op::mType;
using Op::mVType;

struct mVar {
	//Variable object
	mVar(mType type, string id) :type(type), id(id) {}
	mType type;
	string id;
};

class tSub;
class tRoot;
/// Grammar Tree node object
/// Grammar Tree use terminator and nonterminator as node, use each production as child type
class GrammarTree {
public:
	virtual ~GrammarTree() = default;
	virtual void changeid(int id);									//changes tNoVars::id
	virtual Op::NonTerm type();										//return nonterminator's type
	virtual int state();											//return tState::state
	virtual void addTree(GrammarTree* t);							//add Tree node
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v);		//return variable declare, use for Subs
	virtual void extractlabel(map<string, GrammarTree*>& labels);	//return labels, use for declare
	virtual Token* getToken();										//return tTerminator::token
	virtual mType getType();
	virtual int getLineNo();
	//类型检查，包括类型匹配，检查变量声明，处理break，计算goto标签等
	virtual mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock);
	//依据rank值对Token*操作
	virtual GrammarTree* typeChange(int rank);
	//output
	virtual void Output(fSub& inses);
};

//为入栈所使用的状态标记
class tState : public GrammarTree {
public:
	explicit tState(int state);
	int state() override;
private:
	const int _state;
};

//树叶
class tTerminator :public GrammarTree {
public:
	explicit tTerminator(Token* t);
	~tTerminator();
	Token* getToken() override;
	GrammarTree* typeChange(int rank) override;
	int getLineNo() override;
private:
	Token* t;
};

//types
/*class tType : public GrammarTree {
public:
	explicit tType(mType t);
	Op::NonTerm type() override;
	mType getType() override;
private:
	mType t;
};*/

//subv
class tSubVars :public GrammarTree {
public:
	tSubVars() = default;
	tSubVars(mType type, string id);
	void emplaceVar(mType type, string id, int lineNo);
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
	void setDeclType(mType type);
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
private:
	mType typedecl;
	vector<tuple<string, int, GrammarTree*>> varsi;	//逆序储存
};

//其余tree
class tNoVars :public GrammarTree {
public:
	/* almost all statement and expression and etc. node
	 * id saves the type
	 * branchs saves all child node
	 * opID saves reloaded operator id
	 */
	tNoVars(int id, int lineNo);
	~tNoVars();
	void changeid(int id) override;
	template<class ... Types>
	tNoVars(int id, int lineNo, Types&& ... args) : id(id), lineNo(lineNo), branchs({ args... }) {}
	template<class ... Types>
	tNoVars(mVType type, int id, int lineNo, Types&& ... args) : _type(type), id(id), lineNo(lineNo), branchs({ args... }) {}
	Op::NonTerm type() override;
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& labels) override;
	int getLineNo() override;
	GrammarTree* typeChange(int rank) override;
	void Output(fSub& inses) override;
private:
	int id;
	int lineNo;
	mVType _type;
	int opID;						//重载后的运算符id，同时储存insID和modeID（modeID储存幂数*(-1)，即-7代表1<<7）
	vector<GrammarTree*> branchs;	//全部逆序储存，因为产生式都是右递归的
	void LiteralCal();				//字面量计算优化
	void exprTypeCheck(Op::TokenType typ, tSub* sub, tRoot* subs, GrammarTree* whileBlock);
};

//stmts，因为要储存map<label, stmt*>
class tStmts :public GrammarTree {
public:
	~tStmts();
	Op::NonTerm type() override;
	void insertlabel(string s, int lineNo, GrammarTree* t);
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& l) override;
	void Output(fSub& inses) override;
private:
	vector<tuple<string, int, GrammarTree*>> labels;
	list<GrammarTree*> branchs;		//全部逆序储存，因为产生式都是右递归的
};

//单个Sub，不会出现在规约栈中所以不重载type
class tSub : public GrammarTree {
public:
	tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts);
	~tSub();
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void insertDecl(map<string, pair<mType, vector<mType>>>& m) const;
	mVar* checkVar(const string& id);
	GrammarTree* checkLabel(const string& id);
	int getLineNo() override;
	fSub Output();
private:
	const string name;
	int lineNo;
	vector<mType> varpara;				//全部逆序储存
	mType typeReturn;					//目前版本均为void，以后可能扩展至含返回值的函数
	vector<mVar> vardecl;				//全部逆序储存
	map<string, GrammarTree*> labels;
	GrammarTree* stmts;
};

//根节点
class tRoot : public GrammarTree {
public:
	~tRoot();
	Op::NonTerm type() override;
	void addSub(tSub* s);
	pair<mType, vector<mType>>* checkSub(const string& id);
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
private:
	map<string, pair<mType, vector<mType>>> subdecl;
	vector<tSub*> subs;
};