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
#include <string>
#include "Misc.h"
#include "Misc2.h"

using namespace std;
using Op::mType;
using Op::mVType;

//变量
struct mVar {
	mVar(mType type, string id) :type(type), id(id) {}
	mType type;
	string id;
};

class tSub;
class tRoot;
struct StmtOutputContext;
struct SubOutputContext;
class LvalueResult;
class RvalueResult;
//语法树节点类型
//语法树以终结符与非终结符为节点，以每个产生式为子类型
class GrammarTree {
public:
	virtual ~GrammarTree() = default;
	virtual void changeid(int id);
	virtual Op::NonTerm type() const;
	virtual int state();
	virtual void addTree(GrammarTree* t);
	//返回需要展开的vdecl
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v);
	virtual void extractlabel(map<string, GrammarTree*>& labels);
	//取数
	virtual Token* getToken();
	virtual mType getType();
	virtual int getLineNo();
	//类型检查，包括类型匹配，检查变量声明，处理break，计算goto标签等
	virtual mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock);
	//依据rank值对Token*操作
	virtual GrammarTree* typeChange(int rank);
	virtual bool isLabel() const;
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
	Token * t;
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
	Op::NonTerm type() const override;
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
	Op::NonTerm type() const override;
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
	~tNoVars();
	tNoVars(int id, int lineNo);
	void changeid(int id) override;
	template<class ... Types>
	tNoVars(int id, int lineNo, Types&& ... args) : id(id), lineNo(lineNo), branchs({ args... }) {}
	template<class ... Types>
	tNoVars(mVType type, int id, int lineNo, Types&& ... args) : _type(type), id(id), lineNo(lineNo), branchs({ args... }) {}
	Op::NonTerm type() const override;
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& labels) override;
	int getLineNo() override;
	GrammarTree* typeChange(int rank) override;
	mVType get_mVType() const;
	void OutputStmt(SubOutputContext& sub_ctx) const;
	shared_ptr<LvalueResult> OutputLvalueExpr(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false) const;
	shared_ptr<RvalueResult> OutputRvalueExpr(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false) const;
	shared_ptr<LvalueResult> OutputLvalueExprf(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false) const;
	shared_ptr<RvalueResult> OutputRvalueExprf(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false) const;
private:
	int id;
	int lineNo;
	mVType _type;
	int opID;						//重载后的运算符id
	vector<GrammarTree*> branchs;	//全部逆序储存，因为产生式都是右递归的
	void LiteralCal();				//字面量计算优化
	void exprTypeCheck(Op::TokenType typ, tSub* sub, tRoot* subs, GrammarTree* whileBlock);
};

/// <summary>
/// Label statement node in the grammar tree.
/// stmt->id :
/// </summary>
class tLabel final :public GrammarTree {
public:
	~tLabel();
	tLabel(const string& name);
	Op::NonTerm type() const override;
	// TODO: Finish tLabel.
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	bool isLabel() const override;
	string getName() const;
	void Output(SubOutputContext& sub_ctx) const;
private:
	string name;
};

//stmts，因为要储存map<label, stmt*>
class tStmts :public GrammarTree {
public:
	~tStmts();
	Op::NonTerm type() const override;
	void insertlabel(string s, int lineNo, GrammarTree* t);
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& l) override;
	void Output(SubOutputContext& sub_ctx) const;
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
	string getDecoratedName() const;
	fSub Output(const tRoot& root) const;
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
	Op::NonTerm type() const override;
	void addSub(tSub* s);
	pair<mType, vector<mType>>* checkSub(const string& id);
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	string getSubDecoratedName(const string& id) const;
	fRoot Output() const;
private:
	map<string, pair<mType, vector<mType>>> subdecl;
	vector<tSub*> subs;
};
