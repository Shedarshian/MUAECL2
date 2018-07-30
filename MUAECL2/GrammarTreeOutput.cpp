#include "stdafx.h"
#include <stack>
#include <algorithm>
#include "GrammarTree.h"
#include "NameDecorator.h"

using namespace std;

struct StmtOutputContext;
struct SubOutputContext;
class RvalueResult;
class DiscardedRvalueResult;
class StackRvalueResult;
class ParametersRvalueResult;
class LvalueResult;
class DiscardedLvalueResult;
class StackAddrLvalueResult;
class ParametersLvalueResult;

struct StmtOutputContext final {
	StmtOutputContext();
	~StmtOutputContext();
	/// <summary>
	/// The current stack pointer relative to the stack pointer at the beginning of this statement.
	/// Each raw ECL stack value counts as 1.
	/// Should never be less than zero.
	/// Note that the program doesn't keep track of the relative stack pointer for direct instruction call statements.
	/// </summary>
	int32_t stackptr_rel_current = 0;
};

struct SubOutputContext final {
	SubOutputContext(const tRoot* root);
	~SubOutputContext();
	/// <summary>Pointer to the tRoot object that contains the sub.</summary>
	const tRoot* root = nullptr;
	/// <summary>
	/// The count of local variable IDs currently used.
	/// Also the next free local variable ID to be used.
	/// </summary>
	uint32_t count_var = 0;
	/// <summary>A map from declared local variable names to corresponding local variable IDs.</summary>
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
	/// <summary>
	/// A stack that stores the target ID to jump to when a "break;" statement is encountered.
	/// Push to set a value. Pop to restore the previous value.
	/// </summary>
	stack<uint32_t> stack_id_target_break;
	/// <summary>
	/// A stack that stores the target ID to jump to when a "continue;" statement is encountered.
	/// Push to set a value. Pop to restore the previous value.
	/// </summary>
	stack<uint32_t> stack_id_target_continue;
	/// <summary>
	/// A map from label IDs to corresponding target IDs.
	/// Filled when outputting label statements.
	/// </summary>
	unordered_map<string, uint32_t> map_label_target;
	/// <summary>
	/// A stack that stores the current difficulty mask.
	/// Push to set a value. Pop to restore the previous value.
	/// Outputting programs except the outmost one (the one that corresponds to the whole ECL subroutine)
	/// must use a value that contains only bits that are included in the previous value as the new value,
	/// i.e. they must perform a bitwise AND on the value returned by <c>top()</c> and use the result as the new value.
	/// Also, when outputting programs generate an ECL instruction, they must use the current value stored in this stack as the difficulty mask.
	/// </summary>
	stack<uint8_t> stack_difficulty_mask;
	/// <summary>
	/// Insert an ECL instruction.
	/// </summary>
	/// <param name="stmt_ctx">The statement output context of the statement of which this ECL instruction is in.</param>
	/// <param name="id">The ECL instruction ID.</param>
	/// <param name="paras">
	/// The parameters of the ECL instruction.
	/// You must use pointer to dynamically created <c>Parameter</c> objects.
	/// Once inserted, the instruction object manages the pointers and will delete the <c>Parameter</c> objects upon deletion.
	/// </param>
	/// <param name="stackptr_delta">
	/// The net change of the ECL stack pointer, in ECL stack values, of this instruction, EXCEPT of those caused by <c>Parameter_stack</c> parameters.
	/// Changes of the ECL stack pointer caused by <c>Parameter_stack</c> parameters are checked in this function and will be handled internally.
	/// For direct instruction calls, this parameter may be set to 0.
	/// </param>
	/// <returns>An iterator to the newly inserted instruction.</returns>
	inline list<shared_ptr<fSubDataEntry>>::iterator insert_ins(StmtOutputContext& stmt_ctx, int id, const vector<Parameter*>& paras, int32_t stackptr_delta) {
		uint32_t cur_stack_ref_count = 0;
		for (const Parameter* val_param : paras) {
			const Parameter_stack* param_stack = dynamic_cast<const Parameter_stack*>(val_param);
			if (param_stack) {
				int32_t ref_id = param_stack->ref_id;
				if (cur_stack_ref_count > ((uint32_t)-INT32_MIN) - 1) throw(exception("Too large stack reference ID."));
				if (ref_id < 0 && -ref_id == cur_stack_ref_count + 1) ++cur_stack_ref_count;
			}
		}
		stackptr_delta -= cur_stack_ref_count;
		list<shared_ptr<fSubDataEntry>>::iterator ret = this->list_data_entry.insert(this->it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new Ins(id, paras, this->stack_difficulty_mask.top())));
		stmt_ctx.stackptr_rel_current += stackptr_delta;
		return ret;
	}
	inline list<shared_ptr<fSubDataEntry>>::iterator insert_dummyins_target(uint32_t id_target) {
		return this->list_data_entry.insert(this->it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new DummyIns_Target(id_target)));
	}
};

/// <summary>
/// An rvalue evaluation result.
/// An rvalue result must be consumed only when the relative stack pointer is equal to the one when it's created,
/// or, if multiple results are consumed simultaneously, when the relative stack pointer goes through the one when it's created.
/// </summary>
class RvalueResult abstract {
public:
	RvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type) : type(type) {}
	virtual ~RvalueResult() = default;
	/// <summary>Get the type of the result.</summary>
	virtual Op::mType GetType() const { return this->type; }
	/// <summary>
	/// Output instructions that discard this result.
	/// This consumes the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
	/// <summary>
	/// Output instructions that pushes this result to the top of the ECL stack.
	/// This consumes the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual shared_ptr<StackRvalueResult> ToStackRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
	/// <summary>
	/// Output instructions that make the result available as ECL instruction parameter(s) of the corresponding type.
	/// This consumes the result when an instruction that contains the parameter is inserted.
	/// To generate correct raw code, parameters of an instruction from left to right must correspond to rvalue results pushed from latter to earlier.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual vector<shared_ptr<Parameter>> ToParameters(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, int32_t stackptr_rel_parameval) const = 0;
	/// <summary>
	/// Output instructions that duplicate the result.
	/// Duplicating the result counts as creating a result.
	/// This does NOT consume the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual shared_ptr<RvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
protected:
	Op::mType type;
};

/// <summary>
/// An rvalue evaluation result that has been discarded and cannot be recovered.
/// Nothing has to be done for the result to be consumed.
/// </summary>
class DiscardedRvalueResult : public RvalueResult {
public:
	DiscardedRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type) : RvalueResult(sub_ctx, stmt_ctx, type) {}
	virtual ~DiscardedRvalueResult() = default;
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {}
	virtual shared_ptr<StackRvalueResult> ToStackRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		throw(ErrDesignApp("DiscardedRvalueResult::ToStackRvalueResult : discarded results cannot be recovered"));
	}
	virtual vector<shared_ptr<Parameter>> ToParameters(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, int32_t stackptr_rel_parameval) const override {
		throw(ErrDesignApp("DiscardedRvalueResult::ToParameters : discarded results cannot be recovered"));
	}
	virtual shared_ptr<RvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->type));
	}
};

/// <summary>
/// An rvalue evaluation result that's at the top of the ECL stack when being created.
/// Popping the value out of the ECL stack consumes the result.
/// </summary>
class StackRvalueResult : public RvalueResult {
public:
	StackRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type) : RvalueResult(sub_ctx, stmt_ctx, type), stackptr_rel(stmt_ctx.stackptr_rel_current) {}
	virtual ~StackRvalueResult() = default;
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		if (this->stackptr_rel != stmt_ctx.stackptr_rel_current) throw(ErrDesignApp("StackRvalueResult::DiscardResult : The current relative stack pointer doesn't match the relative stack pointer when this result is being created"));
		switch (this->type) {
		case Op::mType::Void:
			break;
		case Op::mType::Int: {
			uint32_t id_var_dummy = sub_ctx.count_var++;
			sub_ctx.insert_ins(stmt_ctx, 43, { new Parameter_variable(id_var_dummy, false) }, -1);
			break;
		}
		case Op::mType::Float: {
			uint32_t id_var_dummy = sub_ctx.count_var++;
			sub_ctx.insert_ins(stmt_ctx, 45, { new Parameter_variable(id_var_dummy, true) }, -1);
			break;
		}
		default:
			throw(ErrDesignApp("StackRvalueResult::DiscardResult : unknown type"));
		}
	}
	virtual shared_ptr<StackRvalueResult> ToStackRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		if (stmt_ctx.stackptr_rel_current != this->stackptr_rel) throw(ErrDesignApp("StackAddrRvalueResult::ToStackRvalueResult : The current relative stack pointer doesn't match the relative stack pointer when this result is being created"));
		return shared_ptr<StackRvalueResult>(new StackRvalueResult(*this));
	}
	virtual vector<shared_ptr<Parameter>> ToParameters(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, int32_t stackptr_rel_parameval) const override {
		switch (this->type) {
		case Op::mType::Void:
			return vector<shared_ptr<Parameter>>();
		case Op::mType::Int: {
			return { shared_ptr<Parameter>(new Parameter_stack(this->GetStackId(sub_ctx, stmt_ctx, stackptr_rel_parameval), false)) };
		}
		case Op::mType::Float: {
			return { shared_ptr<Parameter>(new Parameter_stack(this->GetStackId(sub_ctx, stmt_ctx, stackptr_rel_parameval), true)) };
		}
		default:
			throw(ErrDesignApp("StackRvalueResult::ToParameters : unknown type"));
		}
	}
	virtual shared_ptr<RvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		switch (this->type) {
		case Op::mType::Void:
			return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
		case Op::mType::Int: {
			sub_ctx.insert_ins(stmt_ctx, 2005, { new Parameter_int(-this->GetStackId(sub_ctx, stmt_ctx, stmt_ctx.stackptr_rel_current)) }, 1);
			return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
		}
		case Op::mType::Float: {
			sub_ctx.insert_ins(stmt_ctx, 2005, { new Parameter_int(-this->GetStackId(sub_ctx, stmt_ctx, stmt_ctx.stackptr_rel_current)) }, 1);
			return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
		}
		default:
			throw(ErrDesignApp("StackRvalueResult::Duplicate : unknown type"));
		}
	}
protected:
	int32_t stackptr_rel;
private:
	int32_t GetStackId(const SubOutputContext& sub_ctx, const StmtOutputContext& stmt_ctx, int32_t stackptr_rel_parameval) const {
		return this->stackptr_rel - stackptr_rel_parameval - 1;
	}
};

/// <summary>
/// An rvalue evaluation result that represents one or more ECL parameter(s).
/// This includes local variables, environment variables, literals, etc.
/// However, <c>Parameter_stack</c> &amp; <c>Parameter_call</c> may not be used.
/// </summary>
class ParametersRvalueResult : public RvalueResult {
public:
	ParametersRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type, const vector<shared_ptr<Parameter>>& params) : RvalueResult(sub_ctx, stmt_ctx, type), params(params) {
		switch (type) {
		case Op::mType::Void:
			if (params.size() != 0) throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : Void : wrong parameter count"));
			break;
		case Op::mType::Int:
			if (params.size() != 1) throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : Void : wrong parameter count"));
			if (params[0]->isFloat()) throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : type mismatch"));
			break;
		case Op::mType::Float:
			if (params.size() != 1) throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : Void : wrong parameter count"));
			if (!params[0]->isFloat()) throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : type mismatch"));
			break;
		default:
			throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : unknown type"));
		}
	}
	virtual ~ParametersRvalueResult() = default;
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {}
	virtual shared_ptr<StackRvalueResult> ToStackRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		switch (this->type) {
		case Op::mType::Void:
			return shared_ptr<StackRvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
		case Op::mType::Int: {
			sub_ctx.insert_ins(stmt_ctx, 42, { this->params[0]->Duplicate() }, 1);
			return shared_ptr<StackRvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
		}
		case Op::mType::Float: {
			sub_ctx.insert_ins(stmt_ctx, 44, { this->params[0]->Duplicate() }, 1);
			return shared_ptr<StackRvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
		}
		default:
			throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : unknown type"));
		}
	}
	virtual vector<shared_ptr<Parameter>> ToParameters(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, int32_t stackptr_rel_parameval) const override {
		switch (this->type) {
		case Op::mType::Void:
			return vector<shared_ptr<Parameter>>();
		case Op::mType::Int: {
			return vector<shared_ptr<Parameter>>(params);
		}
		case Op::mType::Float: {
			return vector<shared_ptr<Parameter>>(params);
		}
		default:
			throw(ErrDesignApp("ParametersRvalueResult::ToParameters : unknown type"));
		}
	}
	virtual shared_ptr<RvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		return shared_ptr<RvalueResult>(new ParametersRvalueResult(sub_ctx, stmt_ctx, this->type, this->params));
	}
protected:
	vector<shared_ptr<Parameter>> params;
};

/// <summary>
/// An lvalue evaluation result.
/// An lvalue result must be consumed only when the relative stack pointer is equal to the one when it's created,
/// or, if multiple results are consumed simultaneously, when the relative stack pointer goes through the one when it's created.
/// </summary>
class LvalueResult abstract {
public:
	LvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type) : type(type) {}
	virtual ~LvalueResult() = default;
	/// <summary>Get the type of the result.</summary>
	virtual Op::mType GetType() const { return this->type; }
	/// <summary>
	/// Output instructions that discard this result.
	/// This consumes the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
	/// <summary>
	/// Output instructions that pushes the address of this result to the top of the ECL stack.
	/// This consumes the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual shared_ptr<StackAddrLvalueResult> ToStackAddrLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
	/// <summary>
	/// Output instructions that duplicate the result.
	/// Duplicating the result counts as creating a result.
	/// This does NOT consume the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual shared_ptr<LvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
	/// <summary>
	/// Output instructions that convert this result to an rvalue result that represents the value of this result.
	/// This consumes the result.
	/// Do NOT call this after consuming this result.
	/// </summary>
	virtual shared_ptr<RvalueResult> ToRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) = 0;
	/// <summary>
	/// Output instructions that assign the rvalue result <paramref name="rvres" /> to this lvalue result.
	/// This consumes both this result and <paramref name="rvres" />.
	/// <paramref name="rvres" /> is consumed BEFORE this result. Thus, it must be created AFTER this result.
	/// Do NOT call this after consuming this result or <paramref name="rvres" />.
	/// </summary>
	virtual void Assign(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, shared_ptr<RvalueResult> rvres) = 0;
protected:
	Op::mType type;
};

/// <summary>
/// An lvalue evaluation result that has been discarded and cannot be recovered.
/// Nothing has to be done for the result to be consumed.
/// </summary>
class DiscardedLvalueResult : public LvalueResult {
public:
	DiscardedLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type) : LvalueResult(sub_ctx, stmt_ctx, type) {}
	virtual ~DiscardedLvalueResult() = default;
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {}
	virtual shared_ptr<StackAddrLvalueResult> ToStackAddrLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		throw(ErrDesignApp("DiscardedLvalueResult::ToStackAddrLvalueResult : discarded results cannot be recovered"));
	}
	virtual shared_ptr<LvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->type));
	}
	virtual shared_ptr<RvalueResult> ToRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->type));
	}
	virtual void Assign(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, shared_ptr<RvalueResult> rvres) override {
		throw(ErrDesignApp("DiscardedLvalueResult::Assign : discarded results cannot be recovered"));
	}
};

/// <summary>
/// An lvalue evaluation result whose address is at the top of the ECL stack when being created.
/// Popping the address out of the ECL stack consumes the result.
/// </summary>
class StackAddrLvalueResult : public LvalueResult {
public:
	StackAddrLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type) : LvalueResult(sub_ctx, stmt_ctx, type), stackptr_rel(stmt_ctx.stackptr_rel_current) {}
	virtual ~StackAddrLvalueResult() = default;
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		if (this->stackptr_rel != stmt_ctx.stackptr_rel_current) throw(ErrDesignApp("StackAddrLvalueResult::DiscardResult : The current relative stack pointer doesn't match the relative stack pointer when this result is being created"));
		uint32_t id_var_dummy = sub_ctx.count_var++;
		sub_ctx.insert_ins(stmt_ctx, 43, { new Parameter_variable(id_var_dummy, false) }, -1);
	}
	virtual shared_ptr<StackAddrLvalueResult> ToStackAddrLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		if (stmt_ctx.stackptr_rel_current != this->stackptr_rel) throw(ErrDesignApp("StackAddrLvalueResult::ToStackAddrLvalueResult : The current relative stack pointer doesn't match the relative stack pointer when this result is being created"));
		return shared_ptr<StackAddrLvalueResult>(new StackAddrLvalueResult(*this));
	}
	virtual shared_ptr<LvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		sub_ctx.insert_ins(stmt_ctx, 2005, { new Parameter_int(-this->GetStackId(sub_ctx, stmt_ctx, stmt_ctx.stackptr_rel_current)) }, 1);
		return shared_ptr<LvalueResult>(new StackAddrLvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
	}
	virtual shared_ptr<RvalueResult> ToRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		switch (this->type) {
		case Op::mType::Void:
			return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
		case Op::mType::Int: {
			sub_ctx.insert_ins(stmt_ctx, 2100, {}, 0);
			return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
		}
		case Op::mType::Float: {
			sub_ctx.insert_ins(stmt_ctx, 2101, {}, 0);
			return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
		}
		default:
			throw(ErrDesignApp("StackAddrLvalueResult::ToRvalueResult : unknown type"));
		}
	}
	virtual void Assign(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, shared_ptr<RvalueResult> rvres) override {
		if (rvres->GetType() != this->type) throw(ErrDesignApp("StackAddrLvalueResult::Assign : rvres->GetType() != this->type"));
		rvres->ToStackRvalueResult(sub_ctx, stmt_ctx);
		switch (this->type) {
		case Op::mType::Void:
			break;
		case Op::mType::Int: {
			sub_ctx.insert_ins(stmt_ctx, 2102, {}, -2);
			break;
		}
		case Op::mType::Float: {
			sub_ctx.insert_ins(stmt_ctx, 2103, {}, -2);
			break;
		}
		default:
			throw(ErrDesignApp("StackAddrLvalueResult::Assign : unknown type"));
		}
	}
protected:
	int32_t stackptr_rel;
private:
	int32_t GetStackId(const SubOutputContext& sub_ctx, const StmtOutputContext& stmt_ctx, int32_t stackptr_rel_parameval) const {
		return this->stackptr_rel - stackptr_rel_parameval - 1;
	}
};

/// <summary>
/// An lvalue evaluation result that represents one or more ECL parameter(s).
/// This includes local variables, environment variables, etc.
/// However, <c>Parameter_stack</c>, <c>Parameter_call</c> and literals may not be used.
/// </summary>
class ParametersLvalueResult : public LvalueResult {
public:
	ParametersLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, Op::mType type, const vector<shared_ptr<Parameter>>& params) : LvalueResult(sub_ctx, stmt_ctx, type), params(params) {
		switch (type) {
		case Op::mType::Void:
			if (params.size() != 0) throw(ErrDesignApp("ParametersLvalueResult::ToStackRvalueResult : Void : wrong parameter count"));
			break;
		case Op::mType::Int:
			if (params.size() != 1) throw(ErrDesignApp("ParametersLvalueResult::ToStackRvalueResult : Void : wrong parameter count"));
			if (params[0]->isFloat()) throw(ErrDesignApp("ParametersLvalueResult::ToStackRvalueResult : type mismatch"));
			break;
		case Op::mType::Float:
			if (params.size() != 1) throw(ErrDesignApp("ParametersLvalueResult::ToStackRvalueResult : Void : wrong parameter count"));
			if (!params[0]->isFloat()) throw(ErrDesignApp("ParametersLvalueResult::ToStackRvalueResult : type mismatch"));
			break;
		default:
			throw(ErrDesignApp("ParametersRvalueResult::ToStackRvalueResult : unknown type"));
		}
	}
	virtual ~ParametersLvalueResult() = default;
	virtual void DiscardResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {}
	virtual shared_ptr<StackAddrLvalueResult> ToStackAddrLvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		sub_ctx.insert_ins(stmt_ctx, 2105, { this->params[0]->Duplicate() }, 1);
		return shared_ptr<StackAddrLvalueResult>(new StackAddrLvalueResult(sub_ctx, stmt_ctx, this->type));
	}
	virtual shared_ptr<LvalueResult> Duplicate(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		return shared_ptr<LvalueResult>(new ParametersLvalueResult(sub_ctx, stmt_ctx, this->type, this->params));
	}
	virtual shared_ptr<RvalueResult> ToRvalueResult(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx) override {
		return shared_ptr<RvalueResult>(new ParametersRvalueResult(sub_ctx, stmt_ctx, this->type, this->params));
	}
	virtual void Assign(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, shared_ptr<RvalueResult> rvres) override {
		if (rvres->GetType() != this->type) throw(ErrDesignApp("StackAddrLvalueResult::Assign : rvres->GetType() != this->type"));
		rvres->ToStackRvalueResult(sub_ctx, stmt_ctx);
		switch (this->type) {
		case Op::mType::Void:
			break;
		case Op::mType::Int: {
			sub_ctx.insert_ins(stmt_ctx, 43, { this->params[0]->Duplicate() }, -1);
			break;
		}
		case Op::mType::Float: {
			sub_ctx.insert_ins(stmt_ctx, 45, { this->params[0]->Duplicate() }, -1);
			break;
		}
		default:
			throw(ErrDesignApp("StackAddrLvalueResult::Assign : unknown type"));
		}
	}
protected:
	vector<shared_ptr<Parameter>> params;
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

static inline tNoVars* cast_to_insv(GrammarTree* p) {
	if (p->type() != Op::NonTerm::insv) throw(ErrDesignApp("cast_to_insv : p->type() != Op::NonTerm::insv"));
	tNoVars* insv = dynamic_cast<tNoVars*>(p);
	if (!insv) throw(ErrDesignApp("cast_to_insv : cannot cast p to type \"tNoVars*\""));
	return insv;
}

static inline const tNoVars* cast_to_insv(const GrammarTree* p) {
	if (p->type() != Op::NonTerm::insv) throw(ErrDesignApp("cast_to_insv : p->type() != Op::NonTerm::insv"));
	const tNoVars* insv = dynamic_cast<const tNoVars*>(p);
	if (!insv) throw(ErrDesignApp("cast_to_insv : cannot cast p to type \"const tNoVars*\""));
	return insv;
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

static inline shared_ptr<StackRvalueResult> stack_rvalue_int_expr_output(const GrammarTree* p, SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool no_check_valuetype = false, bool is_root_expr = false) {
	const tNoVars* expr = cast_to_expr(p);
	if (expr->get_mVType().valuetype != Op::LRvalue::rvalue) throw(ErrDesignApp("rvalue_int_expr_output : expr->get_mVType().valuetype != Op::LRvalue::rvalue"));
	if (expr->get_mVType().type != Op::mType::Int) throw(ErrDesignApp("rvalue_int_expr_output : expr->get_mVType().type != Op::mType::Int"));
	return expr->OutputRvalueExpr(sub_ctx, stmt_ctx, false, no_check_valuetype, is_root_expr)->ToStackRvalueResult(sub_ctx, stmt_ctx);
}

static inline shared_ptr<StackRvalueResult> stack_rvalue_int_exprf_output(const GrammarTree* p, SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool no_check_valuetype = false, bool is_root_expr = false) {
	const tNoVars* exprf = cast_to_exprf(p);
	if (exprf->get_mVType().valuetype != Op::LRvalue::rvalue) throw(ErrDesignApp("rvalue_int_exprf_output : exprf->get_mVType().valuetype != Op::LRvalue::rvalue"));
	if (exprf->get_mVType().type != Op::mType::Int) throw(ErrDesignApp("rvalue_int_exprf_output : exprf->get_mVType().type != Op::mType::Int"));
	return exprf->OutputRvalueExprf(sub_ctx, stmt_ctx, false, no_check_valuetype, is_root_expr)->ToStackRvalueResult(sub_ctx, stmt_ctx);
}

void tNoVars::OutputStmt(SubOutputContext& sub_ctx) const {
	if (this->type() != Op::NonTerm::stmt) throw(ErrDesignApp("tNoVars::OutputStmt : this->type() != Op::NonTerm::stmt"));
	StmtOutputContext stmt_ctx;
	switch (this->id) {
	case 2:// stmt->expr ;
	{
		const tNoVars* expr = cast_to_expr(this->branchs[0]);
		switch (expr->_type.valuetype) {
		case Op::LRvalue::lvalue:
			expr->OutputLvalueExpr(sub_ctx, stmt_ctx, true, false, true)->DiscardResult(sub_ctx, stmt_ctx);
			break;
		case Op::LRvalue::rvalue:
			expr->OutputRvalueExpr(sub_ctx, stmt_ctx, true, false, true)->DiscardResult(sub_ctx, stmt_ctx);
			break;
		default:
			throw(ErrDesignApp("tNoVars::OutputStmt : id=2 : unknown valuetype"));
		}
		break;
	}
	case 4:// stmt->if ( expr ) stmt else stmt
	{
		uint32_t id_target_f = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		stack_rvalue_int_expr_output(this->branchs[2], sub_ctx, stmt_ctx, false, true);
		sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
		stmt_output(this->branchs[1], sub_ctx);
		sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
		sub_ctx.insert_dummyins_target(id_target_f);
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.insert_dummyins_target(id_target_after);
		break;
	}
	case 5:// stmt->if ( expr ) stmt
	{
		uint32_t id_target_after = sub_ctx.count_target++;
		stack_rvalue_int_expr_output(this->branchs[1], sub_ctx, stmt_ctx, false, true);
		sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, -1);
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.insert_dummyins_target(id_target_after);
		break;
	}
	case 6:// stmt->while ( expr ) stmt
	{
		uint32_t id_target_expr = sub_ctx.count_target++;
		uint32_t id_target_stmt = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_expr), new Parameter_int(0) }, 0);
		sub_ctx.insert_dummyins_target(id_target_stmt);
		sub_ctx.stack_id_target_break.push(id_target_after);
		sub_ctx.stack_id_target_continue.push(id_target_expr);
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.stack_id_target_continue.pop();
		sub_ctx.stack_id_target_break.pop();
		sub_ctx.insert_dummyins_target(id_target_expr);
		stack_rvalue_int_expr_output(this->branchs[1], sub_ctx, stmt_ctx, false, true);
		sub_ctx.insert_ins(stmt_ctx, 14, { new Parameter_jmp(id_target_stmt), new Parameter_int(0) }, -1);
		sub_ctx.insert_dummyins_target(id_target_after);
		break;
	}
	case 7:// stmt->for ( exprf ) stmt
	{
		uint32_t id_var_loopvar = sub_ctx.count_var++;
		uint32_t id_target_expr = sub_ctx.count_target++;
		uint32_t id_target_stmt = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		stack_rvalue_int_expr_output(this->branchs[1], sub_ctx, stmt_ctx, false, true);
		sub_ctx.insert_ins(stmt_ctx, 43, { new Parameter_variable(id_var_loopvar, false) }, -1);
		sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_expr), new Parameter_int(0) }, 0);
		sub_ctx.insert_dummyins_target(id_target_stmt);
		sub_ctx.stack_id_target_break.push(id_target_after);
		sub_ctx.stack_id_target_continue.push(id_target_expr);
		stmt_output(this->branchs[0], sub_ctx);
		sub_ctx.stack_id_target_continue.pop();
		sub_ctx.stack_id_target_break.pop();
		sub_ctx.insert_dummyins_target(id_target_expr);
		sub_ctx.insert_ins(stmt_ctx, 78, { new Parameter_variable(id_var_loopvar, false) }, 1);
		sub_ctx.insert_ins(stmt_ctx, 14, { new Parameter_jmp(id_target_stmt), new Parameter_int(0) }, -1);
		sub_ctx.insert_dummyins_target(id_target_after);
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
		sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target), new Parameter_int(0) }, 0);
		break;
	}
	case 9:// stmt->{ stmts }
	{
		cast_to_stmts(this->branchs[0])->Output(sub_ctx);
		break;
	}
	case 10:// stmt->break;
	{
		if (sub_ctx.stack_id_target_break.empty()) throw(ErrDesignApp("tNoVars::OutputStmt : id=10 : invalid \"break;\" statement"));
		sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(sub_ctx.stack_id_target_break.top()), new Parameter_int(0) }, 0);
		break;
	}
	case 11:// stmt->continue;
	{
		if (sub_ctx.stack_id_target_continue.empty()) throw(ErrDesignApp("tNoVars::OutputStmt : id=11 : invalid \"continue;\" statement"));
		sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(sub_ctx.stack_id_target_continue.top()), new Parameter_int(0) }, 0);
		break;
	}
	case 29:// stmt->thread id ( insv ) ;
	{
		const tNoVars* insv = cast_to_insv(this->branchs[0]);
		// From right to left.
		vector<shared_ptr<RvalueResult>> vec_result;
		for_each(insv->branchs.cbegin(), insv->branchs.cend(),
			[&sub_ctx, &stmt_ctx, &vec_result](const GrammarTree* val_branch) {
				const tNoVars* exprf = cast_to_exprf(val_branch);
				vec_result.push_back(exprf->OutputRvalueExprf(sub_ctx, stmt_ctx, false));
			}
		);
		string subname_decorated;
		try {
			subname_decorated = sub_ctx.root->getSubDecoratedName(this->branchs[1]->getToken()->getId());
		} catch (out_of_range&) {
			subname_decorated.clear();
		}
		if (!subname_decorated.empty()) {
			// From left to right.
			vector<Parameter_call> vec_param;
			for_each(vec_result.crbegin(), vec_result.crend(),
				[&sub_ctx, &stmt_ctx, &vec_param](const shared_ptr<RvalueResult>& val_result) {
					vector<shared_ptr<Parameter>> vec_param_result = val_result->ToParameters(sub_ctx, stmt_ctx, stmt_ctx.stackptr_rel_current);
					switch (val_result->GetType()) {
					case Op::mType::Void: {
						if (vec_param_result.size() != 0) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : vec_param_result.size() wrong"));
						break;
					}
					case Op::mType::Int: {
						if (vec_param_result.size() != 1) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : vec_param_result.size() wrong"));
						vec_param.emplace_back(vec_param_result[0], false, false);
						break;
					}
					case Op::mType::Float: {
						if (vec_param_result.size() != 1) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : vec_param_result.size() wrong"));
						vec_param.emplace_back(vec_param_result[0], true, true);
						break;
					}
					default:
						throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : unknown type"));
					}
				}
			);
			vector<Parameter*> paras({ new Parameter_string(subname_decorated) });
			for (const Parameter_call& val_param : vec_param) {
				paras.push_back(val_param.Duplicate());
			}
			sub_ctx.insert_ins(stmt_ctx, 15, paras, 0);
			paras.clear();
			break;
		}
	}
	case 30:// stmt->do stmt while (expr);
	{
		uint32_t id_target_expr = sub_ctx.count_target++;
		uint32_t id_target_stmt = sub_ctx.count_target++;
		uint32_t id_target_after = sub_ctx.count_target++;
		sub_ctx.insert_dummyins_target(id_target_stmt);
		sub_ctx.stack_id_target_break.push(id_target_after);
		sub_ctx.stack_id_target_continue.push(id_target_expr);
		stmt_output(this->branchs[1], sub_ctx);
		sub_ctx.stack_id_target_continue.pop();
		sub_ctx.stack_id_target_break.pop();
		sub_ctx.insert_dummyins_target(id_target_expr);
		stack_rvalue_int_expr_output(this->branchs[0], sub_ctx, stmt_ctx, false, true);
		sub_ctx.insert_ins(stmt_ctx, 14, { new Parameter_jmp(id_target_stmt), new Parameter_int(0) }, -1);
		sub_ctx.insert_dummyins_target(id_target_after);
		break;
	}
	case 31:// stmt->__rawins { data }
	{
		if (this->branchs[0]->type() != Op::NonTerm::data) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : this->branchs[0]->type() != Op::NonTerm::data"));
		tNoVars* data = dynamic_cast<tNoVars*>(this->branchs[0]);
		if (!data) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : cannot cast this->branchs[0] to type \"tNoVars*\""));
		for_each(data->branchs.crbegin(), data->branchs.crend(),
			[&sub_ctx, &stmt_ctx](GrammarTree* val_branch) {
				if (val_branch->type() != Op::NonTerm::insdata) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : val_branch->type() != Op::NonTerm::insdata"));
				tNoVars* insdata = dynamic_cast<tNoVars*>(val_branch);
				if (!insdata) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : cannot cast val_branch to type \"tNoVars*\""));
				if (insdata->branchs.size() < 5) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : too few numbers in insdata"));
				// From left to right.
				vector<Token*> vec_num;
				vec_num.reserve(insdata->branchs.size());
				for_each(insdata->branchs.crbegin(), insdata->branchs.crend(),
					[&vec_num](GrammarTree* val_branch_insdata) {
						vec_num.push_back(val_branch_insdata->getToken());
					}
				);
				if (*vec_num[0]->getInt() < 0 || *vec_num[0]->getInt() > UINT16_MAX) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : invalid id"));
				uint16_t id = *vec_num[0]->getInt() & ~(uint16_t)0;
				if (*vec_num[1]->getInt() < 0 || *vec_num[1]->getInt() > UINT8_MAX) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : invalid difficulty mask"));
				uint8_t difficulty_mask = *vec_num[1]->getInt() & ~(uint8_t)0;
				uint32_t post_pop_count = *vec_num[2]->getInt();
				if (*vec_num[3]->getInt() < 0 || *vec_num[3]->getInt() > UINT8_MAX) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : invalid parameter count"));
				uint8_t param_count = *vec_num[3]->getInt() & ~(uint8_t)0;
				if (*vec_num[4]->getInt() < 0 || *vec_num[4]->getInt() > UINT16_MAX) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : invalid parameter mask"));
				uint16_t param_mask = *vec_num[4]->getInt() & ~(uint16_t)0;
				string str_raw_params;
				for (size_t i_num_param = 5; i_num_param < vec_num.size(); ++i_num_param) {
					if (*vec_num[i_num_param]->getInt() < 0 || *vec_num[i_num_param]->getInt() > UCHAR_MAX) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=31 : invalid parameter byte"));
					str_raw_params.push_back(*vec_num[i_num_param]->getInt() & ~(unsigned char)0);
				}
				sub_ctx.list_data_entry.insert(sub_ctx.it_list_data_entry_curpos, shared_ptr<fSubDataEntry>(new RawIns(id, difficulty_mask, post_pop_count, param_count, param_mask, str_raw_params)));
			}
		);
		break;
	}
	default:
		throw(ErrDesignApp("tNoVars::OutputStmt : unknown stmt id"));
	}
	if (stmt_ctx.stackptr_rel_current) throw(ErrDesignApp("tNoVars::OutputStmt : unbalanced stack at the end of the statement"));
}

shared_ptr<LvalueResult> tNoVars::OutputLvalueExpr(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype, bool is_root_expr) const {
	if (this->type() != Op::NonTerm::expr) throw(ErrDesignApp("tNoVars::OutputLvalueExpr : this->type() != Op::NonTerm::expr"));
	if (!no_check_valuetype && this->_type.valuetype != Op::LRvalue::lvalue) throw(ErrDesignApp("tNoVars::OutputLvalueExpr : this->_type.valuetype != Op::LRvalue::lvalue"));
	switch (this->id) {
	case 22:// expr->id
	{
		if (sub_ctx.map_var_id_var.count(this->branchs[0]->getToken()->getId())) {
			// Declared local variable:
			uint32_t id_var = sub_ctx.map_var_id_var.at(this->branchs[0]->getToken()->getId());
			switch (this->_type.type) {
			case Op::mType::Void:
				return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
			case Op::mType::Int:
				if (discard_result) {
					return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				} else {
					return shared_ptr<LvalueResult>(new ParametersLvalueResult(sub_ctx, stmt_ctx, Op::mType::Int, vector<shared_ptr<Parameter>>({ shared_ptr<Parameter>(new Parameter_variable(id_var, false)) })));
				}
			case Op::mType::Float:
				if (discard_result) {
					return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
				} else {
					return shared_ptr<LvalueResult>(new ParametersLvalueResult(sub_ctx, stmt_ctx, Op::mType::Float, vector<shared_ptr<Parameter>>({ shared_ptr<Parameter>(new Parameter_variable(id_var, true)) })));
				}
			default:
				throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=22 : unknown type"));
			}
		} else if (ReadIns::globalVariable.count(this->branchs[0]->getToken()->getId())) {
			// Global variable:
			const pair<int, ReadIns::NumType>* val_global_var = &ReadIns::globalVariable.at(this->branchs[0]->getToken()->getId());
			switch (this->_type.type) {
			case Op::mType::Int:
				if (discard_result) {
					uint32_t id_var_dummy = sub_ctx.count_var++;
					sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_env(val_global_var->first, false) }, 1);
					sub_ctx.insert_ins(stmt_ctx, 43, { new Parameter_variable(id_var_dummy, false) }, -1);
					return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				} else {
					return shared_ptr<LvalueResult>(new ParametersLvalueResult(sub_ctx, stmt_ctx, Op::mType::Int, vector<shared_ptr<Parameter>>({ shared_ptr<Parameter>(new Parameter_env(val_global_var->first, false)) })));
				}
			case Op::mType::Float:
				if (discard_result) {
					uint32_t id_var_dummy = sub_ctx.count_var++;
					sub_ctx.insert_ins(stmt_ctx, 44, { new Parameter_env(val_global_var->first, true) }, 1);
					sub_ctx.insert_ins(stmt_ctx, 45, { new Parameter_variable(id_var_dummy, true) }, -1);
					return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
				} else {
					return shared_ptr<LvalueResult>(new ParametersLvalueResult(sub_ctx, stmt_ctx, Op::mType::Float, vector<shared_ptr<Parameter>>({ shared_ptr<Parameter>(new Parameter_env(val_global_var->first, true)) })));
				}
			default:
				throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=22 : unknown type"));
			}
		} else {
			throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=22 : undefined identifier"));
		}
		break;
	}
	case 25:// expr->Unary_op expr
	{
		switch (this->branchs[1]->getToken()->type()) {
		case Op::TokenType::Deref:
			if (cast_to_expr(this->branchs[0])->_type.type != Op::mType::Int) throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=25 : Deref : unknown address type"));
			if (discard_result) {
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type));
			} else {
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				return shared_ptr<LvalueResult>(new StackAddrLvalueResult(sub_ctx, stmt_ctx, this->_type.type));
			}
		default:
			throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=25 : unknown unary operator"));
		}
		break;
	}
	case 26:// expr->expr Binary_op expr
	{
		static const unordered_map<Op::TokenType, int> map_ins_id_op_int({
			{ Op::TokenType::PlusEqual, 50 },
			{ Op::TokenType::MinusEqual, 52 },
			{ Op::TokenType::TimesEqual, 54 },
			{ Op::TokenType::DividesEqual, 56 },
			{ Op::TokenType::ModEqual, 58 },
			{ Op::TokenType::BitOrEqual, 76 },
			{ Op::TokenType::BitAndEqual, 77 },
			{ Op::TokenType::BitXorEqual, 75 },
			});
		static const unordered_map<Op::TokenType, int> map_ins_id_op_float({
			{ Op::TokenType::PlusEqual, 51 },
			{ Op::TokenType::MinusEqual, 53 },
			{ Op::TokenType::TimesEqual, 55 },
			{ Op::TokenType::DividesEqual, 57 }
			});
		switch (this->branchs[1]->getToken()->type()) {
		case Op::TokenType::Equal: {
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			shared_ptr<LvalueResult> lvres_l = cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, false);
			shared_ptr<LvalueResult> lvres_assign = lvres_l;
			if (!discard_result) lvres_assign = lvres_l->Duplicate(sub_ctx, stmt_ctx);
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			lvres_assign->Assign(sub_ctx, stmt_ctx, shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, this->_type.type)));
			return discard_result ? shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type)) : lvres_l;
		}
		case Op::TokenType::PlusEqual:[[fallthrough]];
		case Op::TokenType::MinusEqual: {
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			shared_ptr<LvalueResult> lvres_l = cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, false);
			shared_ptr<LvalueResult> lvres_assign = lvres_l;
			if (!discard_result) lvres_assign = lvres_l->Duplicate(sub_ctx, stmt_ctx);
			lvres_l->Duplicate(sub_ctx, stmt_ctx)->ToRvalueResult(sub_ctx, stmt_ctx)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			shared_ptr<RvalueResult> rvres;
			switch (this->_type.type) {
			case Op::mType::Int:
				sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
				rvres = shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			case Op::mType::Float:
				sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_float.at(this->branchs[1]->getToken()->type()), {}, -1);
				rvres = shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
			default:
				throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : unknown type"s).c_str()));
			}
			lvres_assign->Assign(sub_ctx, stmt_ctx, rvres);
			return discard_result ? shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type)) : lvres_l;
		}
		case Op::TokenType::TimesEqual:[[fallthrough]];
		case Op::TokenType::DividesEqual: {
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			shared_ptr<LvalueResult> lvres_l = cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, false);
			shared_ptr<LvalueResult> lvres_assign = lvres_l;
			if (!discard_result) lvres_assign = lvres_l->Duplicate(sub_ctx, stmt_ctx);
			lvres_l->Duplicate(sub_ctx, stmt_ctx)->ToRvalueResult(sub_ctx, stmt_ctx)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			shared_ptr<RvalueResult> rvres;
			switch (this->_type.type) {
			case Op::mType::Int:
				if (cast_to_expr(this->branchs[0])->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
				sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
				rvres = shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			case Op::mType::Float:
				if (cast_to_expr(this->branchs[0])->_type.type != Op::mType::Float) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
				sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_float.at(this->branchs[1]->getToken()->type()), {}, -1);
				rvres = shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
			default:
				throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : unknown type"s).c_str()));
			}
			lvres_assign->Assign(sub_ctx, stmt_ctx, rvres);
			return discard_result ? shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type)) : lvres_l;
		}
		case Op::TokenType::ModEqual:[[fallthrough]];
		case Op::TokenType::BitOrEqual:[[fallthrough]];
		case Op::TokenType::BitAndEqual:[[fallthrough]];
		case Op::TokenType::BitXorEqual: {
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			shared_ptr<LvalueResult> lvres_l = cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, false);
			shared_ptr<LvalueResult> lvres_assign = lvres_l;
			if (!discard_result) lvres_assign = lvres_l->Duplicate(sub_ctx, stmt_ctx);
			lvres_l->Duplicate(sub_ctx, stmt_ctx)->ToRvalueResult(sub_ctx, stmt_ctx)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
			shared_ptr<RvalueResult> rvres(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			lvres_assign->Assign(sub_ctx, stmt_ctx, rvres);
			return discard_result ? shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type)) : lvres_l;
		}
		case Op::TokenType::LogicalOrEqual: {
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			shared_ptr<LvalueResult> lvres_l = cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, false);
			shared_ptr<LvalueResult> lvres_assign = lvres_l;
			if (!discard_result) lvres_assign = lvres_l->Duplicate(sub_ctx, stmt_ctx);
			uint32_t id_target_check_r = sub_ctx.count_target++;
			uint32_t id_target_f = sub_ctx.count_target++;
			uint32_t id_target_after = sub_ctx.count_target++;
			lvres_l->Duplicate(sub_ctx, stmt_ctx)->ToRvalueResult(sub_ctx, stmt_ctx)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_check_r), new Parameter_int(0) }, -1);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(1) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
			sub_ctx.insert_dummyins_target(id_target_check_r);
			stack_rvalue_int_expr_output(this->branchs[0], sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(1) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
			sub_ctx.insert_dummyins_target(id_target_f);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(0) }, 1);
			sub_ctx.insert_dummyins_target(id_target_after);
			shared_ptr<RvalueResult> rvres(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			lvres_assign->Assign(sub_ctx, stmt_ctx, rvres);
			return discard_result ? shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type)) : lvres_l;
		}
		case Op::TokenType::LogicalAndEqual: {
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputLvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			shared_ptr<LvalueResult> lvres_l = cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, false);
			shared_ptr<LvalueResult> lvres_assign = lvres_l;
			if (!discard_result) lvres_assign = lvres_l->Duplicate(sub_ctx, stmt_ctx);
			uint32_t id_target_f = sub_ctx.count_target++;
			uint32_t id_target_after = sub_ctx.count_target++;
			lvres_l->Duplicate(sub_ctx, stmt_ctx)->ToRvalueResult(sub_ctx, stmt_ctx)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
			stack_rvalue_int_expr_output(this->branchs[0], sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(1) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
			sub_ctx.insert_dummyins_target(id_target_f);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(0) }, 1);
			sub_ctx.insert_dummyins_target(id_target_after);
			shared_ptr<RvalueResult> rvres(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			lvres_assign->Assign(sub_ctx, stmt_ctx, rvres);
			return discard_result ? shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type)) : lvres_l;
		}
		default:
			throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=26 : unknown operator"));
		}
		break;
	}
	case 27:// expr->expr [ expr ]
	{
		if (cast_to_expr(this->branchs[2])->_type.type != Op::mType::Int) throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=27 : unknown array type"));
		if (cast_to_expr(this->branchs[0])->_type.type != Op::mType::Int) throw(ErrDesignApp("tNoVars::OutputLvalueExpr : id=27 : unknown index type"));
		if (discard_result) {
			cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
			return shared_ptr<LvalueResult>(new DiscardedLvalueResult(sub_ctx, stmt_ctx, this->_type.type));
		} else {
			cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(get_count_id_var(this->_type.type) * sizeof(uint32_t)) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 54, {}, -1);
			sub_ctx.insert_ins(stmt_ctx, 50, {}, -1);
			return shared_ptr<LvalueResult>(new StackAddrLvalueResult(sub_ctx, stmt_ctx, this->_type.type));
		}
		break;
	}
	default:
		throw(ErrDesignApp("tNoVars::OutputLvalueExpr : unknown expr id"));
	}
}

shared_ptr<LvalueResult> tNoVars::OutputLvalueExprf(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype, bool is_root_expr) const {
	if (this->type() != Op::NonTerm::exprf) throw(ErrDesignApp("tNoVars::OutputLvalueExprf : this->type() != Op::NonTerm::exprf"));
	if (!no_check_valuetype && this->_type.valuetype != Op::LRvalue::lvalue) throw(ErrDesignApp("tNoVars::OutputLvalueExprf : this->_type.valuetype != Op::LRvalue::lvalue"));
	switch (this->id) {
	case 20:// exprf->expr
	{
		return cast_to_expr(this->branchs[0])->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result);
	}
	case 21:// exprf->expr : expr : expr : expr
	{
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF1);
		if (cast_to_expr(this->branchs[3])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputLvalueExprf : id=21 : cast_to_expr(this->branchs[3])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[3])->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackAddrLvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF2);
		if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputLvalueExprf : id=21 : cast_to_expr(this->branchs[3])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[2])->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackAddrLvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF4);
		if (cast_to_expr(this->branchs[1])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputLvalueExprf : id=21 : cast_to_expr(this->branchs[3])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[1])->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackAddrLvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF8);
		if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputLvalueExprf : id=21 : cast_to_expr(this->branchs[3])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[0])->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackAddrLvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		return shared_ptr<LvalueResult>(new StackAddrLvalueResult(sub_ctx, stmt_ctx, this->_type.type));
	}
	default:
		throw(ErrDesignApp("tNoVars::OutputLvalueExprf : unknown exprf id"));
	}
}

shared_ptr<RvalueResult> tNoVars::OutputRvalueExpr(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype, bool is_root_expr) const {
	if (this->type() != Op::NonTerm::expr) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : this->type() != Op::NonTerm::expr"));
	if (!no_check_valuetype && this->_type.valuetype != Op::LRvalue::rvalue) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : this->_type.valuetype != Op::LRvalue::rvalue"));
	switch (this->id) {
	case 22:// expr->id
	{
		return this->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result, true)->ToRvalueResult(sub_ctx, stmt_ctx);
	}
	case 23:// expr->num
	{
		switch (this->_type.type) {
		case Op::mType::Void:
			return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
		case Op::mType::Int:
			if (discard_result) {
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			} else {
				int val = *this->branchs[0]->getToken()->getInt();
				return shared_ptr<RvalueResult>(new ParametersRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int, vector<shared_ptr<Parameter>>({ shared_ptr<Parameter>(new Parameter_int(val)) })));
			}
		case Op::mType::Float:
			if (discard_result) {
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
			} else {
				float val = *this->branchs[0]->getToken()->getFloat();
				return shared_ptr<RvalueResult>(new ParametersRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int, vector<shared_ptr<Parameter>>({ shared_ptr<Parameter>(new Parameter_float(val)) })));
			}
		default:
			throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=23 : unknown type"));
		}
		break;
	}
	case 24:// expr->id ( insv )
	{
		if (this->_type.type != Op::mType::Void) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : unknown type"));
		const tNoVars* insv = cast_to_insv(this->branchs[0]);
		// From right to left.
		vector<shared_ptr<RvalueResult>> vec_result;
		for_each(insv->branchs.cbegin(), insv->branchs.cend(),
			[&sub_ctx, &stmt_ctx, &vec_result](const GrammarTree* val_branch) {
				const tNoVars* exprf = cast_to_exprf(val_branch);
				vec_result.push_back(exprf->OutputRvalueExprf(sub_ctx, stmt_ctx, false));
			}
		);
		{
			// Call subroutine.
			string subname_decorated;
			try {
				subname_decorated = sub_ctx.root->getSubDecoratedName(this->branchs[1]->getToken()->getId());
			} catch (out_of_range&) {
				subname_decorated.clear();
			}
			if (!subname_decorated.empty()) {
				// From left to right.
				vector<Parameter_call> vec_param;
				for_each(vec_result.crbegin(), vec_result.crend(),
					[&sub_ctx, &stmt_ctx, &vec_param](const shared_ptr<RvalueResult>& val_result) {
						vector<shared_ptr<Parameter>> vec_param_result = val_result->ToParameters(sub_ctx, stmt_ctx, stmt_ctx.stackptr_rel_current);
						switch (val_result->GetType()) {
						case Op::mType::Void: {
							if (vec_param_result.size() != 0) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : vec_param_result.size() wrong"));
							break;
						}
						case Op::mType::Int: {
							if (vec_param_result.size() != 1) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : vec_param_result.size() wrong"));
							vec_param.emplace_back(vec_param_result[0], false, false);
							break;
						}
						case Op::mType::Float: {
							if (vec_param_result.size() != 1) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : vec_param_result.size() wrong"));
							vec_param.emplace_back(vec_param_result[0], true, true);
							break;
						}
						default:
							throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_result : unknown type"));
						}
					}
				);
				vector<Parameter*> paras({ new Parameter_string(subname_decorated) });
				for (const Parameter_call& val_param : vec_param) {
					paras.push_back(val_param.Duplicate());
				}
				sub_ctx.insert_ins(stmt_ctx, 11, paras, 0);
				paras.clear();
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
			}
		}
		{
			// Directly call ECL instruction.
			pair<
				multimap<string, pair<int, vector<ReadIns::NumType>>>::const_iterator,
				multimap<string, pair<int, vector<ReadIns::NumType>>>::const_iterator
			> range_ins_id(ReadIns::ins.equal_range(this->branchs[1]->getToken()->getId()));
			multimap<string, pair<int, vector<ReadIns::NumType>>>::const_iterator it_ins;
			for (it_ins = range_ins_id.first; it_ins != range_ins_id.second; ++it_ins) {
				// For each instruction slot whose ID is equal to the ID of the call:
				const vector<ReadIns::NumType>* vec_numtype = &it_ins->second.second;
				// If the parameter type specified for this instruction slot is "anything", succeed immediately.
				if (vec_numtype->size() == 1 && vec_numtype->at(0) == ReadIns::NumType::Anything) break;
				vector<shared_ptr<RvalueResult>>::const_reverse_iterator it_result;
				vector<ReadIns::NumType>::const_iterator it_numtype;
				for (
					it_result = vec_result.crbegin(), it_numtype = vec_numtype->cbegin();
					it_result != vec_result.crend() && it_numtype != vec_numtype->cend();
					++it_result) {
					if (*it_numtype == ReadIns::NumType::Call) {
						if ((*it_result)->GetType() == Op::mType::Int
							|| (*it_result)->GetType() == Op::mType::Float)
							continue;
						else
							break;
					}
					// For each rvalue result returned by the exprf outputting function:
					bool is_res_match = false;
					switch ((*it_result)->GetType()) {
					case Op::mType::Int:
						is_res_match = *(it_numtype++) == ReadIns::NumType::Int;
						break;
					case Op::mType::Float:
						is_res_match = *(it_numtype++) == ReadIns::NumType::Float;
						break;
					default:
						throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : unknown actual parameter type"));
					}
					// If the actual parameter doesn't match the formal parameter(s), fail immediately (for this instruction slot).
					if (!is_res_match) break;
				}
				// If the parameters match, succeed immediately.
				if (it_result == vec_result.crend()) {
					if (it_numtype == vec_numtype->cend()) break;
					if (*it_numtype == ReadIns::NumType::Call) break;
				}
			}
			if (it_ins != range_ins_id.second) {
				if (!is_root_expr || !discard_result) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : only an expression that's a root expression and has its result discarded may be an instruction call expression"));
				// If an instruction slot matches the call:
				const vector<ReadIns::NumType>* vec_numtype = &it_ins->second.second;
				vector<ReadIns::NumType>::const_iterator it_numtype;
				// From left to right.
				vector<shared_ptr<Parameter>> vec_param;
				it_numtype = vec_numtype->cbegin();
				for_each(vec_result.crbegin(), vec_result.crend(),
					[&sub_ctx, &stmt_ctx, &vec_numtype, &it_numtype, &vec_param](const shared_ptr<RvalueResult>& val_result) {
						if (it_numtype == vec_numtype->cend()) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : it_numtype == vec_numtype->cend()"));
						vector<shared_ptr<Parameter>> vec_param_result = val_result->ToParameters(sub_ctx, stmt_ctx, stmt_ctx.stackptr_rel_current);
						switch (*it_numtype) {
						case ReadIns::NumType::Int: {
							if (vec_param_result.size() != 1 || vec_param_result.at(0)->isFloat()) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_param_result.size() != 1 || vec_param_result.at(0)->isFloat()"));
							vec_param.reserve(vec_param.size() + vec_param_result.size());
							vec_param.insert(vec_param.cend(), vec_param_result.cbegin(), vec_param_result.cend());
							++it_numtype;
							break;
						}
						case ReadIns::NumType::Float: {
							if (vec_param_result.size() != 1 || !vec_param_result.at(0)->isFloat()) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : vec_param_result.size() != 1 || !vec_param_result.at(0)->isFloat()"));
							vec_param.reserve(vec_param.size() + vec_param_result.size());
							vec_param.insert(vec_param.cend(), vec_param_result.cbegin(), vec_param_result.cend());
							++it_numtype;
							break;
						}
						case ReadIns::NumType::Call: {
							vec_param.reserve(vec_param.size() + vec_param_result.size());
							for (const shared_ptr<Parameter>& val_param_result : vec_param_result) {
								vec_param.push_back(shared_ptr<Parameter>(new Parameter_call(val_param_result, val_param_result->isFloat(), val_param_result->isFloat())));
							}
							break;
						}
						default:
							throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : unknown numtype"));
						}
					}
				);
				vector<Parameter*> paras;
				for (const shared_ptr<Parameter>& val_param : vec_param) {
					paras.push_back(val_param->Duplicate());
				}
				sub_ctx.insert_ins(stmt_ctx, it_ins->second.first, paras, 0);
				paras.clear();
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Void));
			}
		}
		throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=24 : subroutine or instruction not found"));
	}
	case 25:// expr->Unary_op expr
	{
		switch (this->branchs[1]->getToken()->type()) {
		case Op::TokenType::Negative: {
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=25 : Negative : type mismatch"));
			if (discard_result) {
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->_type.type));
			} else {
				switch (this->_type.type) {
				case Op::mType::Int:
					sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(0) }, 1);
					cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
					sub_ctx.insert_ins(stmt_ctx, 52, {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				case Op::mType::Float:
					sub_ctx.insert_ins(stmt_ctx, 44, { new Parameter_float(0) }, 1);
					cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
					sub_ctx.insert_ins(stmt_ctx, 53, {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
				default:
					throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=25 : Negative : unknown type"));
				}
			}
		}
		case Op::TokenType::Not: {
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=25 : Not : type mismatch"));
			if (discard_result) {
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->_type.type));
			} else {
				switch (this->_type.type) {
				case Op::mType::Int:
					cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
					sub_ctx.insert_ins(stmt_ctx, 71, {}, 0);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				case Op::mType::Float:
					cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
					sub_ctx.insert_ins(stmt_ctx, 72, {}, 0);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
				default:
					throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=25 : Negative : unknown type"));
				}
			}
		}
		case Op::TokenType::Deref: {
			return this->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result, true)->ToRvalueResult(sub_ctx, stmt_ctx);
		}
		case Op::TokenType::Address: {
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=25 : Address : this->_type.type != Op::mType::Int"));
			if (discard_result) {
				cast_to_expr(this->branchs[0])->OutputLvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			} else {
				cast_to_expr(this->branchs[0])->OutputLvalueExpr(sub_ctx, stmt_ctx, false)->ToStackAddrLvalueResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			}
		}
		default:
			throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=25 : unknown unary operator"));
		}
		break;
	}
	case 26:// expr->expr Binary_op expr
	{
		static const unordered_map<Op::TokenType, int> map_ins_id_op_int({
			{ Op::TokenType::Or, 73 },
			{ Op::TokenType::And, 74 },
			{ Op::TokenType::BitOr, 76 },
			{ Op::TokenType::BitXor, 75 },
			{ Op::TokenType::BitAnd, 77 },
			{ Op::TokenType::Mod, 58 },
			{ Op::TokenType::EqualTo, 59 },
			{ Op::TokenType::NotEqual, 61 },
			{ Op::TokenType::Greater, 67 },
			{ Op::TokenType::GreaterEqual, 69 },
			{ Op::TokenType::Less, 63 },
			{ Op::TokenType::LessEqual, 65 },
			{ Op::TokenType::Plus, 50 },
			{ Op::TokenType::Minus, 52 },
			{ Op::TokenType::Times, 54 },
			{ Op::TokenType::Divide, 56 }
			});
		static const unordered_map<Op::TokenType, int> map_ins_id_op_float({
			{ Op::TokenType::EqualTo, 60 },
			{ Op::TokenType::NotEqual, 62 },
			{ Op::TokenType::Greater, 68 },
			{ Op::TokenType::GreaterEqual, 70 },
			{ Op::TokenType::Less, 64 },
			{ Op::TokenType::LessEqual, 66 },
			{ Op::TokenType::Plus, 51 },
			{ Op::TokenType::Minus, 53 },
			{ Op::TokenType::Times, 55 },
			{ Op::TokenType::Divide, 57 }
			});
		switch (this->branchs[1]->getToken()->type()) {
		case Op::TokenType::LogicalOr: {
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			uint32_t id_target_check_r = sub_ctx.count_target++;
			uint32_t id_target_f = sub_ctx.count_target++;
			uint32_t id_target_after = sub_ctx.count_target++;
			stack_rvalue_int_expr_output(this->branchs[2], sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_check_r), new Parameter_int(0) }, -1);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(1) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
			sub_ctx.insert_dummyins_target(id_target_check_r);
			stack_rvalue_int_expr_output(this->branchs[0], sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(1) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
			sub_ctx.insert_dummyins_target(id_target_f);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(0) }, 1);
			sub_ctx.insert_dummyins_target(id_target_after);
			if (discard_result) {
				StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int).DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			} else {
				return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			}
		}
		case Op::TokenType::LogicalAnd: {
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			uint32_t id_target_f = sub_ctx.count_target++;
			uint32_t id_target_after = sub_ctx.count_target++;
			stack_rvalue_int_expr_output(this->branchs[2], sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
			stack_rvalue_int_expr_output(this->branchs[0], sub_ctx, stmt_ctx);
			sub_ctx.insert_ins(stmt_ctx, 13, { new Parameter_jmp(id_target_f), new Parameter_int(0) }, -1);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(1) }, 1);
			sub_ctx.insert_ins(stmt_ctx, 12, { new Parameter_jmp(id_target_after), new Parameter_int(0) }, 0);
			sub_ctx.insert_dummyins_target(id_target_f);
			sub_ctx.insert_ins(stmt_ctx, 42, { new Parameter_int(0) }, 1);
			sub_ctx.insert_dummyins_target(id_target_after);
			if (discard_result) {
				StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int).DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			} else {
				return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			}
		}
		case Op::TokenType::Or:[[fallthrough]];
		case Op::TokenType::And:[[fallthrough]];
		case Op::TokenType::BitOr:[[fallthrough]];
		case Op::TokenType::BitXor:[[fallthrough]];
		case Op::TokenType::BitAnd:[[fallthrough]];
		case Op::TokenType::Mod: {
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			if (discard_result) {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			} else {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
				return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			}
		}
		case Op::TokenType::EqualTo:[[fallthrough]];
		case Op::TokenType::NotEqual:[[fallthrough]];
		case Op::TokenType::Greater:[[fallthrough]];
		case Op::TokenType::GreaterEqual:[[fallthrough]];
		case Op::TokenType::Less:[[fallthrough]];
		case Op::TokenType::LessEqual: {
			if (cast_to_expr(this->branchs[0])->_type.type != cast_to_expr(this->branchs[2])->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (this->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : this->_type.type != Op::mType::Int"s).c_str()));
			if (discard_result) {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			} else {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				switch (cast_to_expr(this->branchs[0])->_type.type) {
				case Op::mType::Int:
					sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				case Op::mType::Float:
					sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_float.at(this->branchs[1]->getToken()->type()), {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				default:
					throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : unknown type"s).c_str()));
				}
			}
		}
		case Op::TokenType::Plus:[[fallthrough]];
		case Op::TokenType::Minus: {
			if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
			if (discard_result) {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->_type.type));
			} else {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				switch (this->_type.type) {
				case Op::mType::Int:
					sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				case Op::mType::Float:
					sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_float.at(this->branchs[1]->getToken()->type()), {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
				default:
					throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : unknown type"s).c_str()));
				}
			}
		}
		case Op::TokenType::Times:[[fallthrough]];
		case Op::TokenType::Divide: {
			if (discard_result) {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
				return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->_type.type));
			} else {
				cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				switch (this->_type.type) {
				case Op::mType::Int:
					if (cast_to_expr(this->branchs[0])->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
					if (cast_to_expr(this->branchs[2])->_type.type != Op::mType::Int) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
					sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_int.at(this->branchs[1]->getToken()->type()), {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
				case Op::mType::Float:
					if (cast_to_expr(this->branchs[0])->_type.type != Op::mType::Float) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
					if (cast_to_expr(this->branchs[2])->_type.type != Op::mType::Float) throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : type mismatch"s).c_str()));
					sub_ctx.insert_ins(stmt_ctx, map_ins_id_op_float.at(this->branchs[1]->getToken()->type()), {}, -1);
					return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
				default:
					throw(ErrDesignApp(("tNoVars::OutputRvalueExpr : id=26 : "s + Op::Ch::ToString(this->branchs[1]->getToken()->type()) + " : unknown type"s).c_str()));
				}
			}
		}
		case Op::TokenType::Equal:[[fallthrough]];
		case Op::TokenType::PlusEqual:[[fallthrough]];
		case Op::TokenType::MinusEqual:[[fallthrough]];
		case Op::TokenType::TimesEqual:[[fallthrough]];
		case Op::TokenType::DividesEqual:[[fallthrough]];
		case Op::TokenType::ModEqual:[[fallthrough]];
		case Op::TokenType::LogicalOrEqual:[[fallthrough]];
		case Op::TokenType::LogicalAndEqual:[[fallthrough]];
		case Op::TokenType::BitOrEqual:[[fallthrough]];
		case Op::TokenType::BitAndEqual:[[fallthrough]];
		case Op::TokenType::BitXorEqual: {
			return this->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result, true)->ToRvalueResult(sub_ctx, stmt_ctx);
		}
		default:
			throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=26 : unknown operator"));
		}
		break;
	}
	case 27:// expr->expr [ expr ]
	{
		return this->OutputLvalueExpr(sub_ctx, stmt_ctx, discard_result, true)->ToRvalueResult(sub_ctx, stmt_ctx);
	}
	case 28:// expr->( types ) expr
	{
		if (discard_result) {
			cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, true)->DiscardResult(sub_ctx, stmt_ctx);
			return shared_ptr<RvalueResult>(new DiscardedRvalueResult(sub_ctx, stmt_ctx, this->_type.type));
		} else {
			switch (this->_type.type) {
			case Op::mType::Int:
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				switch (this->branchs[1]->getType()) {
				case Op::mType::Int:
					break;
				case Op::mType::Float:
					sub_ctx.insert_ins(stmt_ctx, 2000, {}, 0);
					break;
				default:
					throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=28 : unknown source type"));
				}
				return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Int));
			case Op::mType::Float:
				cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, false)->ToStackRvalueResult(sub_ctx, stmt_ctx);
				switch (this->branchs[1]->getType()) {
				case Op::mType::Int:
					sub_ctx.insert_ins(stmt_ctx, 2001, {}, 0);
					break;
				case Op::mType::Float:
					break;
				default:
					throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=28 : unknown source type"));
				}
				return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, Op::mType::Float));
			default:
				throw(ErrDesignApp("tNoVars::OutputRvalueExpr : id=28 : unknown destination type"));
			}
		}
		break;
	}
	default:
		throw(ErrDesignApp("tNoVars::OutputRvalueExpr : unknown expr id"));
	}
}

shared_ptr<RvalueResult> tNoVars::OutputRvalueExprf(SubOutputContext& sub_ctx, StmtOutputContext& stmt_ctx, bool discard_result, bool no_check_valuetype, bool is_root_expr) const {
	if (this->type() != Op::NonTerm::exprf) throw(ErrDesignApp("tNoVars::OutputRvalueExprf : this->type() != Op::NonTerm::exprf"));
	if (!no_check_valuetype && this->_type.valuetype != Op::LRvalue::rvalue) throw(ErrDesignApp("tNoVars::OutputRvalueExprf : this->_type.valuetype != Op::LRvalue::rvalue"));
	switch (this->id) {
	case 20:// exprf->expr
	{
		return cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, discard_result);
	}
	case 21:// exprf->expr : expr : expr : expr
	{
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF1);
		if (cast_to_expr(this->branchs[3])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputRvalueExprf : id=21 : cast_to_expr(this->branchs[3])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[3])->OutputRvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackRvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF2);
		if (cast_to_expr(this->branchs[2])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputRvalueExprf : id=21 : cast_to_expr(this->branchs[2])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[2])->OutputRvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackRvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF4);
		if (cast_to_expr(this->branchs[1])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputRvalueExprf : id=21 : cast_to_expr(this->branchs[1])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[1])->OutputRvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackRvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		sub_ctx.stack_difficulty_mask.push(sub_ctx.stack_difficulty_mask.top() & 0xF8);
		if (cast_to_expr(this->branchs[0])->_type.type != this->_type.type) throw(ErrDesignApp("tNoVars::OutputRvalueExprf : id=21 : cast_to_expr(this->branchs[0])->_type.type != this->_type.type"));
		cast_to_expr(this->branchs[0])->OutputRvalueExpr(sub_ctx, stmt_ctx, discard_result)->ToStackRvalueResult(sub_ctx, stmt_ctx);
		sub_ctx.stack_difficulty_mask.pop();
		return shared_ptr<RvalueResult>(new StackRvalueResult(sub_ctx, stmt_ctx, this->_type.type));
	}
	default:
		throw(ErrDesignApp("tNoVars::OutputRvalueExprf : unknown exprf id"));
	}
}

void tLabel::Output(SubOutputContext& sub_ctx) const {
	uint32_t id_target = UINT32_MAX;
	try {
		id_target = sub_ctx.map_label_target.at(this->name);
	} catch (out_of_range&) {
		id_target = sub_ctx.count_target++;
		sub_ctx.map_label_target[this->name] = id_target;
	}
	sub_ctx.insert_dummyins_target(id_target);
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

fSub tSub::Output(const tRoot& root) const {
	SubOutputContext sub_ctx(&root);

	for_each(this->vardecl.crbegin(), this->vardecl.crend(),
		[&sub_ctx](const mVar& val_vardecl) {
			uint32_t id_var = sub_ctx.count_var;
			sub_ctx.count_var += get_count_id_var(val_vardecl.type);
			sub_ctx.map_var_id_var[val_vardecl.id] = id_var;
		}
	);

	sub_ctx.stack_difficulty_mask.push(0xFF);
	cast_to_stmts(this->stmts)->Output(sub_ctx);
	sub_ctx.stack_difficulty_mask.pop();
	vector<shared_ptr<fSubDataEntry>> vec_data_entry(sub_ctx.list_data_entry.cbegin(), sub_ctx.list_data_entry.cend());

	return fSub(this->getDecoratedName(), sub_ctx.count_var, vec_data_entry);
}

string tRoot::getSubDecoratedName(const string& id) const {
	if (ReadIns::defaultList.count(id)) {
		return id;
	} else {
		pair<mType, vector<mType>> val_subdecl;
		val_subdecl = this->subdecl.at(id);
		return NameDecorator::decorateSubName(id, val_subdecl.first, val_subdecl.second);
	}
}

fRoot tRoot::Output() const {
	vector<fSub> fsubs;
	for (const tSub* val_tsub : this->subs)
		fsubs.push_back(val_tsub->Output(*this));
	// TODO: ECLI & ANIM.
	return fRoot(fsubs, vector<string>(), vector<string>());
}

StmtOutputContext::StmtOutputContext() {}

StmtOutputContext::~StmtOutputContext() {}

SubOutputContext::SubOutputContext(const tRoot* root) {
	if (!root) throw(ErrDesignApp("SubOutputContext::SubOutputContext : !root"));
	this->root = root;
	this->it_list_data_entry_curpos = list_data_entry.begin();
}

SubOutputContext::~SubOutputContext() {}
