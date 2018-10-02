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
#include <iostream>
#include <iterator>
#include <type_traits>
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

struct StmtOutputContext;
struct SubOutputContext;
class LvalueResult;
class RvalueResult;

/// Grammar Tree node object
/// Grammar Tree use terminator and nonterminator as node, use each production as child type
class GrammarTree {
public:
	virtual ~GrammarTree() = default;
	virtual void changeid(int id);										//changes tNoVars::id
	virtual Op::NonTerm type() const;									//return nonterminator's type
	virtual int state() const;											//return tState::state
	virtual void addTree(GrammarTree* t);								//add Tree node
	virtual list<GrammarTree*>* extractdecl(vector<mVar>& v);			//return variable declare, use for Subs
	virtual void extractlabel(map<string, GrammarTree*>& labels);	//return labels, use for declare
	virtual Token* getToken() const;									//return tTerminator::token

	virtual mType getType() const;										//return tTerminator "types" type
	virtual int getLineNo() const;
	//类型检查，包括类型匹配，检查变量声明，处理break，计算goto标签等
	virtual mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock);
	//依据rank值对Token*操作
	virtual GrammarTree* typeChange(Op::Rank rank);
	virtual bool isLabel() const;
protected:
	static constexpr bool debug = false;
};

//为入栈所使用的状态标记
/*class tState : public GrammarTree {
public:
	explicit tState(int state);
	int state() const override;
private:
	const int _state;
};*/

//树叶
class tTerminator :public GrammarTree {
public:
	explicit tTerminator(Token* t);
	~tTerminator();
	Token* getToken() const override;
	mType getType() const override;
	GrammarTree* typeChange(Op::Rank rank) override;
	int getLineNo() const override;
private:
	Token * t;
};

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
	friend class tRoot;
};

//vdecl
class tDeclVars :public GrammarTree {
public:
	tDeclVars(string id, int lineNo);
	tDeclVars(string id, int lineNo, GrammarTree* inif);
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
	/* almost all statement and expression and etc. node
	 * id saves the type
	 * branchs saves all child node
	 * opID saves reloaded operator id
	 */
	tNoVars(int id, int lineNo);
	~tNoVars();
	tNoVars(const tNoVars&) = delete;
	tNoVars& operator=(const tNoVars&) = delete;
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
	int getLineNo() const override;
	GrammarTree* typeChange(Op::Rank rank) override;
	mVType get_mVType() const;
	void OutputStmt(SubOutputContext& sub_ctx) const;
	shared_ptr<LvalueResult> OutputLvalueExpr(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false, bool is_root_expr = false) const;
	shared_ptr<RvalueResult> OutputRvalueExpr(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false, bool is_root_expr = false) const;
	shared_ptr<LvalueResult> OutputLvalueExprf(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false, bool is_root_expr = false) const;
	shared_ptr<RvalueResult> OutputRvalueExprf(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false, bool is_root_expr = false) const;
	shared_ptr<RvalueResult> OutputRvalueIni(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false, bool is_root_expr = false) const;
	shared_ptr<RvalueResult> OutputRvalueInif(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype = false, bool is_root_expr = false) const;
private:
	int id;
	int lineNo;
	mVType _type;
	int opID;						//重载后的运算符id，同时储存insID和modeID
	vector<GrammarTree*> branchs;	//全部逆序储存，因为产生式都是右递归的
	void LiteralCal();				//字面量计算优化
	template<typename type_saved, typename _ConstIterator, typename _SequenceContainer>
	typename conditional<is_void<type_saved>::value, Op::mVType, pair<Op::mVType, type_saved*>>::type OverloadCheck(
		_SequenceContainer CheckObjects,
		function<Op::mVType(typename remove_reference<_SequenceContainer>::type::value_type)> TypeCheckFunction,
		function<tuple<type_saved*, Op::mVType, vector<Op::mVType>>(const _ConstIterator&)> GetContainmentsFunction,
		function<void(Op::Rank, typename remove_reference<_SequenceContainer>::type::iterator&)> TypeChangeFunction,
		_ConstIterator& begin_it, const _ConstIterator& end_it);
	//void exprTypeCheck(Op::TokenType typ, tSub* sub, tRoot* subs, GrammarTree* whileBlock);
};

//_ConstIterator requires InputIterator<T>
//_SequenceContainer requires SequenceContainer<T> && is_same<typename T::value_type, GrammarTree*>
//	namely: vector<GrammarTree*>& or vector<GrammarTree**>
//type_saved requires DefaultConstructible<T> || is_void<T>
template<typename type_saved, typename _ConstIterator, typename _SequenceContainer>
typename conditional<is_void<type_saved>::value, Op::mVType, pair<Op::mVType, type_saved*>>::type tNoVars::OverloadCheck(
	_SequenceContainer CheckObjects,
	function<Op::mVType(typename remove_reference<_SequenceContainer>::type::value_type)> TypeCheckFunction,
	function<tuple<type_saved*, Op::mVType, vector<Op::mVType>>(const _ConstIterator&)> GetContainmentsFunction,
	function<void(Op::Rank, typename remove_reference<_SequenceContainer>::type::iterator&)> TypeChangeFunction,
	_ConstIterator& begin_it, const _ConstIterator& end_it) {
	vector<Op::Rank> ranks, ranksmin; Op::Rank rankmin = RANK_MAX;
	ranks.resize(CheckObjects.size());
	type_saved* savedObjectPtr;
	Op::mVType returnType;
	vector<Op::mVType> insvtyp;
	for (auto it = CheckObjects.begin(); it != CheckObjects.end(); ++it)
		insvtyp.push_back(TypeCheckFunction(*it));
	for (; begin_it != end_it; ++begin_it) {
		Op::Rank ranksum;
		auto&&[dataSavedPtr, _returnType, inputTypes] = GetContainmentsFunction(begin_it);
		if (inputTypes.size() != CheckObjects.size())
			continue;
		auto it = ranks.begin();
		auto it1 = inputTypes.cbegin();
		Op::Rank rankSaved;
		for (auto it2 = insvtyp.begin(); it2 != insvtyp.end(); it++, it1++, it2++) {
			Op::Rank rank = Op::mVType::canChangeTo(*it2, *it1);
			if (rank.incorrect()) {
				goto for_end;
			}
			else {
				*it = rank;
				ranksum += rank;
			}
		}
		if (ranksum < rankmin) {
			rankmin = ranksum; ranksmin = ranks;
			savedObjectPtr = dataSavedPtr; returnType = _returnType;
		}
for_end:;
	}
	//无法应用运算符到此类型操作数
	if (rankmin == RANK_MAX) {
		if constexpr(is_void<type_saved>::value)
			return VTYPE(type_error, r);
		else
			return pair<mVType, type_saved*>(VTYPE(type_error, r), savedObjectPtr);
	}
	//依据rank值确定做了些什么变换
	auto rank_it = ranksmin.begin();
	for (auto it = CheckObjects.begin(); it != CheckObjects.end(); rank_it++, it++) {
		TypeChangeFunction(*rank_it, it);
		//*it = (*it)->typeChange(*rank_it);
	}
	if constexpr(is_void<type_saved>::value)
		return returnType;
	else
		return pair<mVType, type_saved*>(returnType, savedObjectPtr);
}

/*template<typename type_saved, typename _ConstIterator>
pair<Op::mVType, type_saved> tNoVars::OverloadCheck(const vector<GrammarTree*>& CheckObjects, function<Op::mVType(GrammarTree*)> TypeCheckFunction, function<tuple<type_saved, Op::mVType, vector<Op::mVType>>(const _ConstIterator&)> GetContainmentsFunction, _ConstIterator& begin_it) {
	vector<Op::Rank> ranks, ranksmin; Op::Rank rankmin = RANK_MAX;
	ranks.resize(CheckObjects.size());
	type_saved savedObjects;
	Op::mVType returnType;
	Op::Rank ranksum;
	auto&&[dataSaved, _returnType, inputTypes] = GetContainmentsFunction(begin_it);
	if (inputTypes.size() != CheckObjects.size()) {
		return make_pair<mVType, type_saved>(VTYPE(type_error, r), savedObjects);
	}
	auto it = ranks.begin();
	auto it1 = inputTypes.cbegin();
	Op::Rank rankSaved;
	for (auto it2 = CheckObjects.cbegin(); it2 != CheckObjects.cend(); it++, it1++, it2++) {
		Op::mVType insvtyp = TypeCheckFunction(*it2);
		if (Op::Rank rank = Op::mVType::canChangeTo(insvtyp, *it1); rank.incorrect()) {
			return make_pair<mVType, type_saved>(VTYPE(type_error, r), savedObjects);
		}
		else {
			*it = rank;
			ranksum += rank;
		}
	}
	if (ranksum < rankmin) {
		rankmin = ranksum; ranksmin = ranks;
		savedObjects = dataSaved; returnType = _returnType;
	}
	//无法应用运算符到此类型操作数
	if (rankmin == RANK_MAX) {
		return make_pair<mVType, type_saved>(VTYPE(type_error, r), savedObjects);
	}
	//依据rank值确定做了些什么变换
	auto rank_it = ranksmin.begin();
	for (auto it = CheckObjects.begin(); it != CheckObjects.end(); rank_it++, it++) {
		*it = (*it)->typeChange(*rank_it);
	}
	return make_pair(returnType, savedObjects);
}

/*void tNoVars::exprTypeCheck(Op::TokenType typ, tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	auto rtype = branchs[0]->TypeCheck(sub, subs, whileBlock);
	decltype(rtype) ltype;
	bool isLiteral = rtype.isLiteral;
	//如果是二元运算符
	if (branchs.size() == 3) {
		ltype = branchs[2]->TypeCheck(sub, subs, whileBlock);
		isLiteral = isLiteral && ltype.isLiteral;
	}
	else if (branchs.size() == 4) {
		for (int i = 1; i < 4; i++)
			branchs[i]->TypeCheck(sub, subs, whileBlock);
		isLiteral = false;
	}
	auto[low, up] = mVType::typeChange.equal_range(typ);
	int minID = INT_MAX;
	int rank[4], ranksum;
	for (; low != up; ++low) {
		auto &[first, second, type, opID] = low->second;
		int ranktemp[4];
		ranktemp[0] = mVType::canChangeTo(rtype, second);
		if (ranktemp[0] < 0) continue;
		ranksum = ranktemp[0];
		if (branchs.size() == 3) {
			ranktemp[1] = mVType::canChangeTo(ltype, first);
			if (ranktemp[1] < 0) continue;
			ranksum += ranktemp[1];
		}
		else if (branchs.size() == 4) {
			ranktemp[1] = mVType::canChangeTo(static_cast<tNoVars*>(branchs[1])->_type, first);
			if (ranktemp[1] < 0) continue;
			ranktemp[2] = mVType::canChangeTo(static_cast<tNoVars*>(branchs[1])->_type, first);
			if (ranktemp[2] < 0) continue;
			ranktemp[3] = mVType::canChangeTo(static_cast<tNoVars*>(branchs[1])->_type, first);
			if (ranktemp[3] < 0) continue;
			ranksum += ranktemp[1] + ranktemp[2] + ranktemp[3];
		}
		if (minID > ranksum) {
			minID = ranksum;
			_type = type; this->opID = opID;
			for (auto i : { 0, 1, 2, 3 })
				rank[i] = ranktemp[i];
		}
	}
	if (minID == INT_MAX)
		throw(ErrTypeOperate(lineNo, ltype, rtype, (branchs.size() == 4) ? "colon"s : branchs[1]->getToken()->debug_out()));
	//依据rank值确定做了些什么变换
	branchs[0] = branchs[0]->typeChange(rank[0]);
	if (branchs.size() == 3)
		branchs[2] = branchs[2]->typeChange(rank[1]);
	else if (branchs.size() == 4)
		for (int i : {1, 2, 3})
			branchs[i] = branchs[i]->typeChange(rank[i]);
	//处理字面量计算优化，需要排除字面量指针的情况（在里面处理）
	if (isLiteral)
		LiteralCal();
}*/


/// <summary>
/// Label statement node in the grammar tree.
/// stmt->id :
/// </summary>
class tLabel final :public GrammarTree {
public:
	~tLabel();
	tLabel(const string& name, int lineNo);
	Op::NonTerm type() const override;
	// TODO: Finish tLabel.
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& l) override;
	bool isLabel() const override;
	string getName() const;
	void Output(SubOutputContext& sub_ctx) const;
private:
	string name;
	int lineNo;
};

//stmts
class tStmts :public GrammarTree {
public:
	tStmts() = default;
	tStmts(list<GrammarTree*>& branchs);
	~tStmts();
	Op::NonTerm type() const override;
	//void insertlabel(string s, int lineNo, GrammarTree* t);
	void addTree(GrammarTree* t) override;
	list<GrammarTree*>* extractdecl(vector<mVar>& v) override;
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void extractlabel(map<string, GrammarTree*>& l) override;
	void Output(SubOutputContext& sub_ctx) const;
private:
	//vector<tuple<string, int, GrammarTree*>> labels;
	list<GrammarTree*> branchs;		//全部逆序储存，因为产生式都是右递归的
};

//单个Sub，不会出现在规约栈中所以不重载type
class tSub : public GrammarTree {
public:
	tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts, bool no_overload);
	~tSub();
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	void insertDecl(multimap<string, tuple<mType, vector<mType>, bool>>& m) const;
	mVar* checkVar(const string& id);
	GrammarTree* checkLabel(const string& id);
	int getLineNo() const override;
	string getDecoratedName() const;
	fSub Output(const tRoot& root) const;
private:
	bool no_overload;					//是否装饰sub名
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
	void addSubDecl(string name, int lineNo, tSubVars* subv, mType typeReturn, bool no_overload);
	decltype(declval<multimap<string, tuple<mType, vector<mType>, bool>>>().equal_range(declval<string>())) checkSub(const string& id);
	mVType TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) override;
	/// <param name="types_param">From right to left.</param>
	string getSubDecoratedName(const string& id, const vector<mType>& types_params) const;
	fRoot Output(const vector<string>& ecli, const vector<string>& anim) const;
private:
	multimap<string, tuple<mType, vector<mType>, bool>> subdecl;
	vector<tSub*> subs;
};
