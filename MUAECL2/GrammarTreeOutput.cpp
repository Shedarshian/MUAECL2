#include "stdafx.h"
#include <stack>
#include <algorithm>
#include "GrammarTree.h"
#include "NameDecorator.h"

using namespace std;

struct SubOutputContext final {
	SubOutputContext();
	~SubOutputContext();
	/// <summary>
	/// The count of local variable IDs currently used.
	/// Also the next free local variable ID to be used.
	/// </summary>
	uint32_t count_var = 0;
	/// <summary>A map from formal parameter indices (0 based) to corresponding local variable IDs.</summary>
	map<uint32_t, uint32_t> map_param_id_var;
	/// <summary>A map from declared local parameter names to corresponding local variable IDs.</summary>
	map<string, uint32_t> map_var_id_var;
	/// <summary>A list that contains sub data entries (instructions and other things).</summary>
	list<shared_ptr<fSubDataEntry>> list_data_entry;
	/// <summary>Position in list_data_entry where sub data entries are inserted.</summary>
	list<shared_ptr<fSubDataEntry>>::const_iterator it_list_data_entry_curpos;
	/// <summary>
	/// The count of target IDs currently used.
	/// Also the next free target ID to be used.
	/// </summary>
	uint32_t count_target = 0;
	/// <summary>A stack that stores the target ID to jump to when a "break;" statement is encountered.</summary>
	stack<uint32_t> stack_id_target_break;
	/// <summary>A stack that stores the target ID to jump to when a "continue;" statement is encountered.</summary>
	stack<uint32_t> stack_id_target_continue;
	/// <summary>
	/// A map from label IDs to corresponding target IDs.
	/// Filled when outputting label statements.
	/// </summary>
	unordered_map<string, uint32_t> map_label_target;
};

/// <summary>Get the count of local variable ID slots (aka local variables in the raw ECL) needed for a value of the specified type.</summary>
static inline uint32_t get_count_id_var(Op::mType type) {
	switch (type) {
	case Op::mType::type_error:
		throw(ErrDesignApp("get_count_id_var : type is Op::mType::type_error"));
	case Op::mType::Void:
		return 0;
	case Op::mType::Int:
		[[fallthrough]];
	case Op::mType::Float:
		return 1;
	case Op::mType::String:
		[[fallthrough]];
	case Op::mType::Point:
		[[fallthrough]];
	case Op::mType::inilist:
		[[fallthrough]];
	default:
		throw(ErrDesignApp("get_count_id_var : unknown type"));
	}
}

static inline tNoVars* cast_to_expr(GrammarTree* p) {
	if (p->type() != Op::NonTerm::expr) throw(ErrDesignApp("cast_to_expr : p->type() != Op::NonTerm::expr"));
	tNoVars* expr = dynamic_cast<tNoVars*>(p);
	if (!expr) throw(ErrDesignApp("cast_to_expr : cannot cast p to type \"tNoVars*\""));
	return expr;
}

static inline const tNoVars* cast_to_expr(const GrammarTree* p) {
	if (p->type() != Op::NonTerm::expr) throw(ErrDesignApp("cast_to_expr : p->type() != Op::NonTerm::expr"));
	const tNoVars* expr = dynamic_cast<const tNoVars*>(p);
	if (!expr) throw(ErrDesignApp("cast_to_expr : cannot cast p to type \"const tNoVars*\""));
	return expr;
}

static inline tNoVars* cast_to_exprf(GrammarTree* p) {
	if (p->type() != Op::NonTerm::exprf) throw(ErrDesignApp("cast_to_exprf : p->type() != Op::NonTerm::exprf"));
	tNoVars* exprf = dynamic_cast<tNoVars*>(p);
	if (!exprf) throw(ErrDesignApp("cast_to_exprf : cannot cast p to type \"tNoVars*\""));
	return exprf;
}

static inline const tNoVars* cast_to_exprf(const GrammarTree* p) {
	if (p->type() != Op::NonTerm::exprf) throw(ErrDesignApp("cast_to_exprf : p->type() != Op::NonTerm::exprf"));
	const tNoVars* exprf = dynamic_cast<const tNoVars*>(p);
	if (!exprf) throw(ErrDesignApp("cast_to_exprf : cannot cast p to type \"const tNoVars*\""));
	return exprf;
}

static inline tNoVars* cast_to_normal_stmt(GrammarTree* p) {
	if (p->type() != Op::NonTerm::stmt) throw(ErrDesignApp("cast_to_normal_stmt : p->type() != Op::NonTerm::stmt"));
	if (p->isLabel()) throw(ErrDesignApp("cast_to_normal_stmt : p->isLabel() returns true"));
	tNoVars* stmt = dynamic_cast<tNoVars*>(p);
	if (!stmt) throw(ErrDesignApp("cast_to_normal_stmt : cannot cast p to type \"tNoVars*\""));
	return stmt;
}

static inline const tNoVars* cast_to_normal_stmt(const GrammarTree* p) {
	if (p->type() != Op::NonTerm::stmt) throw(ErrDesignApp("cast_to_normal_stmt : p->type() != Op::NonTerm::stmt"));
	if (p->isLabel()) throw(ErrDesignApp("cast_to_normal_stmt : p->isLabel() returns true"));
	const tNoVars* stmt = dynamic_cast<const tNoVars*>(p);
	if (!stmt) throw(ErrDesignApp("cast_to_normal_stmt : cannot cast p to type \"const tNoVars*\""));
	return stmt;
}

static inline tLabel* cast_to_label_stmt(GrammarTree* p) {
	if (p->type() != Op::NonTerm::stmt) throw(ErrDesignApp("cast_to_label_stmt : p->type() != Op::NonTerm::stmt"));
	if (!p->isLabel()) throw(ErrDesignApp("cast_to_label_stmt : p->isLabel() returns false"));
	tLabel* stmt = dynamic_cast<tLabel*>(p);
	if (!stmt) throw(ErrDesignApp("cast_to_label_stmt : cannot cast p to type \"tLabel*\""));
	return stmt;
}

static inline const tLabel* cast_to_label_stmt(const GrammarTree* p) {
	if (p->type() != Op::NonTerm::stmt) throw(ErrDesignApp("cast_to_label_stmt : p->type() != Op::NonTerm::stmt"));
	if (!p->isLabel()) throw(ErrDesignApp("cast_to_label_stmt : p->isLabel() returns false"));
	const tLabel* stmt = dynamic_cast<const tLabel*>(p);
	if (!stmt) throw(ErrDesignApp("cast_to_label_stmt : cannot cast p to type \"const tLabel*\""));
	return stmt;
}

static inline tStmts* cast_to_stmts(GrammarTree* p) {
	if (p->type() != Op::NonTerm::stmts) throw(ErrDesignApp("cast_to_stmts : p->type() != Op::NonTerm::stmts"));
	tStmts* stmts = dynamic_cast<tStmts*>(p);
	if (!stmts) throw(ErrDesignApp("cast_to_stmts : cannot cast p to type \"tStmts*\""));
	return stmts;
}

static inline const tStmts* cast_to_stmts(const GrammarTree* p) {
	if (p->type() != Op::NonTerm::stmts) throw(ErrDesignApp("cast_to_stmts : p->type() != Op::NonTerm::stmts"));
	const tStmts* stmts = dynamic_cast<const tStmts*>(p);
	if (!stmts) throw(ErrDesignApp("cast_to_stmts : cannot cast p to type \"const tStmts*\""));
	return stmts;
}

static inline void stmt_output(const GrammarTree* p, SubOutputContext& sub_ctx) {
	if (p->type() != Op::NonTerm::stmt) throw(ErrDesignApp("stmt_output : p->type() != Op::NonTerm::stmt"));
	if (p->isLabel()) {
		const tLabel* stmt = cast_to_label_stmt(p);
		stmt->Output(sub_ctx);
	} else {
		const tNoVars* stmt = cast_to_normal_stmt(p);
		stmt->OutputStmt(sub_ctx);
	}
}

static inline void rvalue_int_expr_output(const GrammarTree* p, SubOutputContext& sub_ctx) {
	const tNoVars* expr = cast_to_expr(p);
	if (expr->get_mVType().valuetype != Op::LRvalue::rvalue) throw(ErrDesignApp("rvalue_int_expr_output : expr->get_mVType().valuetype != Op::LRvalue::rvalue"));
	if (expr->get_mVType().type != Op::mType::Int) throw(ErrDesignApp("rvalue_int_expr_output : expr->get_mVType().type != Op::mType::Int"));
	expr->OutputExpr(sub_ctx, false);
}

static inline void rvalue_int_exprf_output(const GrammarTree* p, SubOutputContext& sub_ctx) {
	const tNoVars* exprf = cast_to_exprf(p);
	if (exprf->get_mVType().valuetype != Op::LRvalue::rvalue) throw(ErrDesignApp("rvalue_int_exprf_output : exprf->get_mVType().valuetype != Op::LRvalue::rvalue"));
	if (exprf->get_mVType().type != Op::mType::Int) throw(ErrDesignApp("rvalue_int_exprf_output : exprf->get_mVType().type != Op::mType::Int"));
	exprf->OutputExprf(sub_ctx, false);
}

void tNoVars::OutputStmt(SubOutputContext& sub_ctx) const {
	if (this->type() != Op::NonTerm::stmt) throw(ErrDesignApp("tNoVars::OutputStmt : this->type() != Op::NonTerm::stmt"));
	switch (this->id) {
	case 2:// stmt->expr ;
	{
		cast_to_expr(this->branchs[0])->OutputExpr(sub_ctx, true);
		break;
	}
	case 4:// stmt->if ( expr ) stmt else stmt
	{
		uint32_t id_target_f = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		rvalue_int_expr_output(this->branchs[2], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(13, { new Parameter_jmp(id_target_f), new Parameter_int(0) })));
		stmt_output(this->branchs[1], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(12, { new Parameter_jmp(id_target_after), new Parameter_int(0) })));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_f)));
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_after)));
		break;
	}
	case 5:// stmt->if ( expr ) stmt
	{
		uint32_t id_target_after = sub_ctx.count_target++;
		rvalue_int_expr_output(this->branchs[1], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(13, { new Parameter_jmp(id_target_after), new Parameter_int(0) })));
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_after)));
		break;
	}
	case 6:// stmt->while ( expr ) stmt
	{
		uint32_t id_target_loop = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_loop)));
		rvalue_int_expr_output(this->branchs[1], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(13, { new Parameter_jmp(id_target_after), new Parameter_int(0) })));
		sub_ctx.stack_id_target_break.push(id_target_after);
		sub_ctx.stack_id_target_continue.push(id_target_loop);
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.stack_id_target_continue.pop();
		sub_ctx.stack_id_target_break.pop();
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(12, { new Parameter_jmp(id_target_loop), new Parameter_int(0) })));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_after)));
		break;
	}
	case 7:// stmt->for ( exprf ) stmt
	{
		uint32_t id_var_loopvar = sub_ctx.count_var++;
		uint32_t id_target_loop = sub_ctx.count_target++;
		uint32_t id_target_cont = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		rvalue_int_exprf_output(this->branchs[1], sub_ctx);
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(43, { new Parameter_variable(id_var_loopvar, false) })));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_loop)));
		sub_ctx.stack_id_target_break.push(id_target_after);
		sub_ctx.stack_id_target_continue.push(id_target_cont);
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.stack_id_target_continue.pop();
		sub_ctx.stack_id_target_break.pop();
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_cont)));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(78, { new Parameter_variable(id_var_loopvar, false) })));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(14, { new Parameter_jmp(id_target_after), new Parameter_int(0) })));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(12, { new Parameter_jmp(id_target_loop), new Parameter_int(0) })));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target_after)));
		break;
	}
	case 8:// stmt->goto id ;
	{
		string name = cast_to_label_stmt(this->branchs[0])->getName();
		uint32_t id_target = UINT32_MAX;
		try {
			id_target = sub_ctx.map_label_target.at(name);
		} catch (out_of_range&) {
			id_target = sub_ctx.count_target++;
			sub_ctx.map_label_target[name] = id_target;
		}
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(12, { new Parameter_jmp(id_target), new Parameter_int(0) })));
		break;
	}
	case 9:// stmt->{ stmts }
	{
		cast_to_stmts(this->branchs[0])->Output(sub_ctx);
		break;
	}
	case 10:// stmt->break;
	{
		if (sub_ctx.stack_id_target_break.empty()) throw(ErrDesignApp("tNoVars::Output : id=10 : invalid \"break;\" statement"));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(12, { new Parameter_jmp(sub_ctx.stack_id_target_break.top()), new Parameter_int(0) })));
		break;
	}
	case 11:// stmt->continue;
	{
		if (sub_ctx.stack_id_target_continue.empty()) throw(ErrDesignApp("tNoVars::Output : id=11 : invalid \"continue;\" statement"));
		sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(12, { new Parameter_jmp(sub_ctx.stack_id_target_continue.top()), new Parameter_int(0) })));
		break;
	}
	default:
		throw(ErrDesignApp("tNoVars::Output : unknown stmt id"));
	}
}

void tNoVars::OutputExpr(SubOutputContext& sub_ctx, bool discard_result) const {
	// TODO: Implement tNoVars::OutputExpr.
}

void tNoVars::OutputExprf(SubOutputContext& sub_ctx, bool discard_result) const {
	// TODO: Implement tNoVars::OutputExprf.
}

void tLabel::Output(SubOutputContext& sub_ctx) const {
	uint32_t id_target = UINT32_MAX;
	try {
		id_target = sub_ctx.map_label_target.at(this->name);
	} catch (out_of_range&) {
		id_target = sub_ctx.count_target++;
		sub_ctx.map_label_target[this->name] = id_target;
	}
	sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target)));
}

void tStmts::Output(SubOutputContext& sub_ctx) const {
	for_each(this->branchs.crbegin(), this->branchs.crend(),
		[&sub_ctx](const GrammarTree* val_branch) {
			stmt_output(val_branch, sub_ctx);
		}
	);
}

string tSub::getDecoratedName() const {
	vector<mType> types_param;
	for_each(this->varpara.crbegin(), this->varpara.crend(),
		[&types_param](mType val_varpara) {
			types_param.push_back(val_varpara);
		}
	);
	return NameDecorator::decorateSubName(this->name, this->typeReturn, types_param);
}

fSub tSub::Output() const {
	SubOutputContext ctx;

	uint32_t i_param = 0;
	for_each(this->varpara.crbegin(), this->varpara.crend(),
		[&ctx, &i_param](mType val_varpara) {
			uint32_t id_var = ctx.count_var;
			ctx.count_var += get_count_id_var(val_varpara);
			ctx.map_param_id_var[i_param++] = id_var;
		}
	);

	for_each(this->vardecl.crbegin(), this->vardecl.crend(),
		[&ctx](const mVar& val_vardecl) {
			uint32_t id_var = ctx.count_var;
			ctx.count_var += get_count_id_var(val_vardecl.type);
			ctx.map_var_id_var[val_vardecl.id] = id_var;
		}
	);

	cast_to_stmts(this->stmts)->Output(ctx);
	vector<shared_ptr<fSubDataEntry>> vec_data_entry(ctx.list_data_entry.cbegin(), ctx.list_data_entry.cend());

	return fSub(this->getDecoratedName(), ctx.count_var, vec_data_entry);
}

fRoot tRoot::Output() const {
	vector<fSub> fsubs;
	for (const tSub* val_tsub : this->subs)
		fsubs.push_back(val_tsub->Output());
	// TODO: ECLI & ANIM.
	return fRoot(fsubs, vector<string>(), vector<string>());
}

SubOutputContext::SubOutputContext() {
	this->it_list_data_entry_curpos = list_data_entry.begin();
}

SubOutputContext::~SubOutputContext() {}
