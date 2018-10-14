#include "Misc3.h"

using namespace std;

DecodedParam::DecodedParam(ParamType param_type) : param_type(param_type) {}

DecodedParam::~DecodedParam() {}

DecodedParam_Int* DecodedParam_Int::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Int) throw(ErrDesignApp("DecodedParam_Int::CastToMe : p->param_type != ParamType::Int"));
	DecodedParam_Int* ret = static_cast<DecodedParam_Int*>(p);
	return ret;
}

DecodedParam_Int::DecodedParam_Int(int32_t val) : DecodedParam(DecodedParam::ParamType::Int), val(val) {}

DecodedParam_Int::~DecodedParam_Int() {}

DecodedParam_Float* DecodedParam_Float::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Float) throw(ErrDesignApp("DecodedParam_Float::CastToMe : p->param_type != ParamType::Float"));
	DecodedParam_Float* ret = static_cast<DecodedParam_Float*>(p);
	return ret;
}

DecodedParam_Float::DecodedParam_Float(float val) : DecodedParam(DecodedParam::ParamType::Float), val(val) {}

DecodedParam_Float::~DecodedParam_Float() {}

DecodedParam_String* DecodedParam_String::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::String) throw(ErrDesignApp("DecodedParam_String::CastToMe : p->param_type != ParamType::String"));
	DecodedParam_String* ret = static_cast<DecodedParam_String*>(p);
	return ret;
}

DecodedParam_String::DecodedParam_String(string str) : DecodedParam(DecodedParam::ParamType::String), val(val) {}

DecodedParam_String::~DecodedParam_String() {}

DecodedParam_Variable* DecodedParam_Variable::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Variable) throw(ErrDesignApp("DecodedParam_Variable::CastToMe : p->param_type != ParamType::Variable"));
	DecodedParam_Variable* ret = static_cast<DecodedParam_Variable*>(p);
	return ret;
}

DecodedParam_Variable::DecodedParam_Variable(uint32_t id_var, bool is_float) : DecodedParam(DecodedParam::ParamType::Variable), id_var(id_var), is_float(is_float) {}

DecodedParam_Variable::~DecodedParam_Variable() {}

DecodedParam_AbnormalVariable* DecodedParam_AbnormalVariable::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::AbnormalVariable) throw(ErrDesignApp("DecodedParam_AbnormalVariable::CastToMe : p->param_type != ParamType::AbnormalVariable"));
	DecodedParam_AbnormalVariable* ret = static_cast<DecodedParam_AbnormalVariable*>(p);
	return ret;
}

DecodedParam_AbnormalVariable::DecodedParam_AbnormalVariable(int32_t ref_id, bool is_float) : DecodedParam(DecodedParam::ParamType::AbnormalVariable), ref_id(ref_id), is_float(is_float) {}

DecodedParam_AbnormalVariable::~DecodedParam_AbnormalVariable() {}

DecodedParam_Stack* DecodedParam_Stack::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Stack) throw(ErrDesignApp("DecodedParam_Stack::CastToMe : p->param_type != ParamType::Stack"));
	DecodedParam_Stack* ret = static_cast<DecodedParam_Stack*>(p);
	return ret;
}

DecodedParam_Stack::DecodedParam_Stack(int32_t ref_id, bool is_float) : DecodedParam(DecodedParam::ParamType::Stack), ref_id(ref_id), is_float(is_float) {}

DecodedParam_Stack::~DecodedParam_Stack() {}

DecodedParam_Env* DecodedParam_Env::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Env) throw(ErrDesignApp("DecodedParam_Env::CastToMe : p->param_type != ParamType::Env"));
	DecodedParam_Env* ret = static_cast<DecodedParam_Env*>(p);
	return ret;
}

DecodedParam_Env::DecodedParam_Env(uint32_t env_id, bool is_float) : DecodedParam(DecodedParam::ParamType::Env), env_id(env_id), is_float(is_float) {}

DecodedParam_Env::~DecodedParam_Env() {}

DecodedParam_Jmp* DecodedParam_Jmp::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Jmp) throw(ErrDesignApp("DecodedParam_Jmp::CastToMe : p->param_type != ParamType::Jmp"));
	DecodedParam_Jmp* ret = static_cast<DecodedParam_Jmp*>(p);
	return ret;
}

DecodedParam_Jmp::DecodedParam_Jmp(uint32_t id_target) : DecodedParam(DecodedParam::ParamType::Jmp), id_target(id_target) {}

DecodedParam_Jmp::~DecodedParam_Jmp() {}

DecodedParam_Call* DecodedParam_Call::CastToMe(DecodedParam* p) {
	if (p->param_type != ParamType::Call) throw(ErrDesignApp("DecodedParam_Call::CastToMe : p->param_type != ParamType::Call"));
	DecodedParam_Call* ret = static_cast<DecodedParam_Call*>(p);
	return ret;
}

DecodedParam_Call::DecodedParam_Call(shared_ptr<DecodedParam>& param, bool is_from_float, bool is_to_float) : DecodedParam(DecodedParam::ParamType::Call), param(param), is_from_float(is_from_float), is_to_float(is_to_float) {}

DecodedParam_Call::~DecodedParam_Call() {}

DecodedSubDataEntry::DecodedSubDataEntry(DataEntryType data_entry_type) : data_entry_type(data_entry_type) {}

DecodedSubDataEntry:: ~DecodedSubDataEntry() {}

DecodedJmpTarget* DecodedJmpTarget::CastToMe(DecodedSubDataEntry* p) {
	if (p->data_entry_type != DataEntryType::JmpTarget) throw(ErrDesignApp("DecodedJmpTarget::CastToMe : p->data_entry_type != DataEntryType::JmpTarget"));
	DecodedJmpTarget* ret = static_cast<DecodedJmpTarget*>(p);
	return ret;
}

DecodedJmpTarget::DecodedJmpTarget(uint32_t id_target) : DecodedSubDataEntry(DecodedSubDataEntry::DataEntryType::JmpTarget), id_target(id_target) {}

DecodedJmpTarget::~DecodedJmpTarget() {}

DecodedIns* DecodedIns::CastToMe(DecodedSubDataEntry* p) {
	if (p->data_entry_type != DataEntryType::Ins) throw(ErrDesignApp("DecodedIns::CastToMe : p->data_entry_type != DataEntryType::Ins"));
	DecodedIns* ret = static_cast<DecodedIns*>(p);
	return ret;
}

DecodedIns::DecodedIns()
	: DecodedSubDataEntry(DecodedSubDataEntry::DataEntryType::Ins), time(UINT32_MAX), id(UINT16_MAX), difficulty_mask(0), post_pop_count(0) {}

DecodedIns::DecodedIns(const DecodedIns& t) : DecodedSubDataEntry(DecodedSubDataEntry::DataEntryType::Ins) {
	this->time = t.time;
	this->id = t.id;
	this->difficulty_mask = t.difficulty_mask;
	this->post_pop_count = t.post_pop_count;
	this->is_rawins = t.is_rawins;
	if (t.is_rawins) {
		this->rawins_params = t.rawins_params ? new rawins_params_t(*t.rawins_params) : nullptr;
	}
	else {
		this->params = t.params ? new vector<shared_ptr<DecodedParam>>(*t.params) : nullptr;
	}
}

DecodedIns::DecodedIns(DecodedIns&& t) : DecodedSubDataEntry(DecodedSubDataEntry::DataEntryType::Ins) {
	this->time = t.time;
	this->id = t.id;
	this->difficulty_mask = t.difficulty_mask;
	this->post_pop_count = t.post_pop_count;
	this->is_rawins = t.is_rawins;
	if (t.is_rawins) {
		this->rawins_params = t.rawins_params;
		t.rawins_params = nullptr;
	}
	else {
		this->params = t.params;
		t.params = nullptr;
	}
}

DecodedIns::~DecodedIns() {
	if (this->is_rawins) {
		if (this->rawins_params) {
			delete this->rawins_params;
			this->rawins_params = nullptr;
		}
	}
	else {
		if (this->params) {
			delete this->params;
			this->params = nullptr;
		}
	}
}

DecoderException::DecoderException(size_t offset) throw() : offset(offset) {}

DecoderException::~DecoderException() throw() {}

size_t DecoderException::GetOffset() const throw() { return this->offset; }

void DecoderException::AdjustOffset(const char* ptr_initial, const char* ptr_initial_inner) throw() {
	ptrdiff_t diff = ptr_initial_inner - ptr_initial;
	this->offset += diff;
}

ErrDecoderUnexpEof::ErrDecoderUnexpEof(const char* ptr, size_t size_buf, const char* ptr_initial) throw() :DecoderException((ptr - ptr_initial) + size_buf) {}

ErrDecoderUnexpEof::~ErrDecoderUnexpEof() throw() {}

const char* ErrDecoderUnexpEof::what() const throw() {
	return "Unexpected end-of-file.";
}

ErrDecoderInvalidEcl::ErrDecoderInvalidEcl(size_t offset) throw()
	: DecoderException(offset) {}

ErrDecoderInvalidEcl::~ErrDecoderInvalidEcl() throw() {}

const char* ErrDecoderInvalidEcl::what() const throw() {
	return "Invalid raw ECL file.";
}

ErrDecoderInvalidIncludes::ErrDecoderInvalidIncludes(size_t offset) throw()
	: DecoderException(offset) {}

ErrDecoderInvalidIncludes::~ErrDecoderInvalidIncludes() throw() {}

const char* ErrDecoderInvalidIncludes::what() const throw() {
	return "Invalid includes block.";
}

ErrDecoderInvalidSub::ErrDecoderInvalidSub(size_t offset) throw()
	: DecoderException(offset) {}

ErrDecoderInvalidSub::~ErrDecoderInvalidSub() throw() {}

const char* ErrDecoderInvalidSub::what() const throw() {
	return "Invalid raw ECL subroutine.";
}

ErrDecoderInvalidParameters::ErrDecoderInvalidParameters(size_t offset) throw()
	: DecoderException(offset) {}

ErrDecoderInvalidParameters::~ErrDecoderInvalidParameters() throw() {}

const char* ErrDecoderInvalidParameters::what() const throw() {
	return "Invalid raw ECL parameter(s).";
}