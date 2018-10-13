#include "stdafx.h"
#include "Misc.h"
#include "Misc2.h"
#include "RawEclGenerator.h"

using namespace std;

/// <summary>
/// Append <c>size</c> bytes of data from <c>src</c> to <c>dst</c>, and increase <c>dst</c> by size.
/// </summary>
static inline char* append_data(char*& dst, const void* src, size_t size) {
	return reinterpret_cast<char*>(memcpy(
		(dst += size) - size,
		src,
		size
	));
}
/// <summary>
/// Append some null bytes to align <c>ptr</c> and <c>size</c> to 4 bytes boundary.
/// </summary>
static inline void align4_data(char*& ptr, size_t size_buf, size_t& size) {
	if (size % 4) {
		uint32_t _dummy = 0;
		if (ptr && size_buf >= size + 4 - (size % 4))
			append_data(ptr, &_dummy, 4 - (size % 4));
		size += 4 - (size % 4);
	}
}

bool Parameter_int::isFloat() const { return false; }

bool Parameter_int::isRefParam() const { return false; }

int32_t Parameter_int::getRefId(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_int::getRefId")); }

size_t Parameter_int::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
	if (ptr && size_buf >= sizeof(int)) {
		append_data(ptr, &this->val, sizeof(int));
	}
	return sizeof(int);
}

bool Parameter_float::isFloat() const { return true; }

bool Parameter_float::isRefParam() const { return false; }

int32_t Parameter_float::getRefId(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_float::getRefId")); }

size_t Parameter_float::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
	if (ptr && size_buf >= sizeof(float)) {
		append_data(ptr, &this->val, sizeof(float));
	}
	return sizeof(float);
}

bool Parameter_variable::isFloat() const { return this->is_float; }

bool Parameter_variable::isRefParam() const { return true; }

int32_t Parameter_variable::getRefId(const SubSerializationContext& sub_ctx) const {
	if (this->id_var >= sub_ctx.count_var) throw(ErrDesignApp("Parameter_variable::getRefId : id_var out of range"));
	if (this->id_var > INT32_MAX / 4) throw(exception("Too many local variables."));
	return this->id_var * 4;
}

size_t Parameter_variable::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	int32_t ref_id = INT32_MAX;
	if (ptr && size_buf >= sizeof(float)) {
		ref_id = this->getRefId(sub_ctx);
	}
	if (this->is_float) {
		static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(float)) {
			// If ref_id is more than 24 bits, storing it in a single-precision floating point value would result in inaccuracy.
			if ((ref_id < 0 ? -ref_id : ref_id) & 0x7F000000) throw(exception("Too many variables."));
#pragma warning(push)
#pragma warning(disable:4244)
			float f_ref_id = ref_id;
#pragma warning(pop)
			append_data(ptr, &f_ref_id, sizeof(float));
		}
		return sizeof(float);
	} else {
		static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(int)) {
			int i_ref_id = ref_id;
			append_data(ptr, &i_ref_id, sizeof(int));
		}
		return sizeof(int);
	}
}

bool Parameter_stack::isFloat() const { return this->is_float; }

bool Parameter_stack::isRefParam() const { return true; }

int32_t Parameter_stack::getRefId(const SubSerializationContext& sub_ctx) const {
	return this->ref_id;
}

size_t Parameter_stack::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	int32_t ref_id = INT32_MAX;
	if (ptr && size_buf >= sizeof(float)) {
		ref_id = this->getRefId(sub_ctx);
	}
	if (this->is_float) {
		static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(float)) {
			// If ref_id is more than 24 bits, storing it in a single-precision floating point value would result in inaccuracy.
			if ((ref_id < 0 ? -ref_id : ref_id) & 0x7F000000) throw(exception("Too large stack reference ID."));
#pragma warning(push)
#pragma warning(disable:4244)
			float f_ref_id = ref_id;
#pragma warning(pop)
			append_data(ptr, &f_ref_id, sizeof(float));
		}
		return sizeof(float);
	} else {
		static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(int)) {
			int i_ref_id = ref_id;
			append_data(ptr, &i_ref_id, sizeof(int));
		}
		return sizeof(int);
	}
}

bool Parameter_env::isFloat() const { return this->is_float; }

bool Parameter_env::isRefParam() const { return true; }

int32_t Parameter_env::getRefId(const SubSerializationContext& sub_ctx) const {
	static_assert(sizeof(int) == sizeof(int32_t), "sizeof(int) is not equal to sizeof(int32_t).");
	return -(int32_t)this->env_id;
}

size_t Parameter_env::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	int32_t ref_id = INT32_MAX;
	if (ptr && size_buf >= sizeof(float)) {
		ref_id = this->getRefId(sub_ctx);
	}
	if (this->is_float) {
		static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(float)) {
			// If ref_id is more than 24 bits, storing it in a single-precision floating point value would result in inaccuracy.
			if ((ref_id < 0 ? -ref_id : ref_id) & 0x7F000000) throw(exception("Too large environment reference ID."));
#pragma warning(push)
#pragma warning(disable:4244)
			float f_ref_id = ref_id;
#pragma warning(pop)
			append_data(ptr, &f_ref_id, sizeof(float));
		}
		return sizeof(float);
	} else {
		static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(int)) {
			int i_ref_id = ref_id;
			append_data(ptr, &i_ref_id, sizeof(int));
		}
		return sizeof(int);
	}
}

bool Parameter_jmp::isFloat() const { return false; }

bool Parameter_jmp::isRefParam() const { return false; }

int32_t Parameter_jmp::getRefId(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_jmp::getRefId")); }

size_t Parameter_jmp::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	if (ptr && size_buf >= sizeof(int32_t)) {
		size_t offs_data_entry_current = 0;
		size_t offs_data_entry_target = 0;
		try {
			offs_data_entry_current = sub_ctx.vec_offs_data_entry.at(sub_ctx.i_data_entry_current);
		} catch (out_of_range&) {
			throw(ErrDesignApp("Parameter_jmp::serialize : current data entry index out of range"));
		}
		try {
			offs_data_entry_target = sub_ctx.map_offs_target.at(this->id_target);
		} catch (out_of_range&) {
			throw(ErrDesignApp("Parameter_jmp::serialize : target not found"));
		}
		if (offs_data_entry_target > INT32_MAX || offs_data_entry_current > INT32_MAX) throw(exception("Too large ECL sub data entry offset."));
		int32_t offs_jmp = (offs_data_entry_target & ~(uint32_t)0) - (offs_data_entry_current & ~(uint32_t)0);
		append_data(ptr, &offs_jmp, sizeof(int32_t));
	}
	return sizeof(int32_t);
}

bool Parameter_string::isFloat() const { return false; }

bool Parameter_string::isRefParam() const { return false; }

int32_t Parameter_string::getRefId(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_string::getRefId")); }

size_t Parameter_string::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	size_t size = 0;
	if (this->str.size() > UINT32_MAX - 4) throw(exception("String parameter too large."));
	uint32_t size_strdata = (this->str.size() & ~(uint32_t)0) / 4 * 4 + 4;
	size += sizeof(uint32_t);
	if (ptr && size_buf >= size) {
		append_data(ptr, &size_strdata, sizeof(uint32_t));
	}
	size += size_strdata * sizeof(char);
	if (ptr && size_buf >= size) {
		unique_ptr<char[]> strdata(new char[size_strdata]());
		memcpy(strdata.get(), this->str.data(), this->str.size() * sizeof(char));
		append_data(ptr, strdata.get(), size_strdata * sizeof(char));
	}
	return size;
}

bool Parameter_call::isFloat() const { return this->is_to_float; }

bool Parameter_call::isRefParam() const { return this->param->isRefParam(); }

int32_t Parameter_call::getRefId(const SubSerializationContext& sub_ctx) const { return this->param->getRefId(sub_ctx); }

size_t Parameter_call::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	size_t size = 0;
	size += sizeof(uint32_t);
	if (ptr && size_buf >= size) {
		uint32_t typeval = (this->is_from_float ? 0x66 : 0x69) | (this->is_to_float ? 0x6600 : 0x6900);
		append_data(ptr, &typeval, sizeof(uint32_t));
	}
	if (this->is_from_float != this->param->isFloat()) throw(ErrDesignApp("Parameter_call::serialize : actual parameter type mismatch"));
	size_t size_param = this->param->serialize(nullptr, 0, sub_ctx);
	size += size_param;
	if (ptr && size_buf >= size) {
		this->param->serialize((ptr += size_param) - size_param, size_param, sub_ctx);
	}
	return size;
}

void fSubDataEntry::set_offs(SubSerializationContext& sub_ctx, size_t offs) const {}

DummyIns_Target::DummyIns_Target(uint32_t id_target)
	: id_target(id_target) {}

size_t DummyIns_Target::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	return 0;
}

void DummyIns_Target::set_offs(SubSerializationContext& sub_ctx, size_t offs) const {
	if (sub_ctx.map_offs_target.count(this->id_target)) throw(ErrDesignApp("DummyIns_Target::set_offs : duplicate targets with the same ID"));
	sub_ctx.map_offs_target[this->id_target] = offs;
}

Ins::Ins(uint16_t id, const vector<Parameter*>& paras, uint8_t difficulty_mask, uint32_t time)
	: id(id), paras(paras), difficulty_mask(difficulty_mask), time(time) {}

Ins::~Ins() {
	for (auto ptr : paras)
		delete ptr;
}

size_t Ins::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	size_t size = 0;

	struct {
		__pragma(pack(push, 1));
		uint32_t time;
		uint16_t id;
		uint16_t size;
		uint16_t param_mask;
		/* The difficulty bitmask.
		*   1111LHNE
		* Bits mean: easy, normal, hard, lunatic. The rest are always set to 1. */
		uint8_t diff_mask;
		/* There doesn't seem to be a way of telling how many parameters there are
		* from the additional data. */
		uint8_t param_count;
		/* From TH13 on, this field stores the number of current stack references
		* in the parameter list. */
		uint32_t post_pop_count;
		__pragma(pack(pop));
	} raw_ecl_ins_hdr;
	char* ptr_raw_ecl_ins_hdr = nullptr;
	size += sizeof(raw_ecl_ins_hdr);
	if (ptr && size_buf >= size) {
		raw_ecl_ins_hdr.time = this->time;
		raw_ecl_ins_hdr.id = this->id;
		//raw_ecl_ins_hdr.size;
		raw_ecl_ins_hdr.param_mask = 0;
		raw_ecl_ins_hdr.diff_mask = this->difficulty_mask;
		//raw_ecl_ins_hdr.param_count;
		//raw_ecl_ins_hdr.post_pop_count;
		ptr_raw_ecl_ins_hdr = append_data(ptr, &raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr));
	}

	std::vector<Parameter*> vec_param(this->paras);
	if (vec_param.size() > 16) throw(exception("Too many parameters."));
	if (ptr && size_buf >= size) {
		raw_ecl_ins_hdr.param_count = vec_param.size() & ~(uint8_t)0;
	}
	raw_ecl_ins_hdr.post_pop_count = 0;
	int i = 0;
	for (const Parameter* val_param : vec_param) {
		size_t size_param = val_param->serialize(nullptr, 0, sub_ctx);
		size += size_param;
		if (ptr && size_buf >= size) {
			if (val_param->isRefParam()) {
				raw_ecl_ins_hdr.param_mask |= 1 << i;
				int32_t ref_id = val_param->getRefId(sub_ctx);
				if (raw_ecl_ins_hdr.post_pop_count > ((uint32_t)-INT32_MIN) - 1) throw(exception("Too large stack reference ID."));
				if (ref_id < 0 && -ref_id == raw_ecl_ins_hdr.post_pop_count + 1) ++raw_ecl_ins_hdr.post_pop_count;
			}
			if (val_param->serialize((ptr += size_param) - size_param, size_param, sub_ctx) != size_param) throw(ErrDesignApp("Inconsistent returned size when calling Parameter::serialize"));
		}
		++i;
	}
	raw_ecl_ins_hdr.post_pop_count <<= 3;

	if (ptr && size_buf >= size) {
		if (size > UINT16_MAX) throw(exception("Too large instruction."));
		raw_ecl_ins_hdr.size = size & ~(uint16_t)0;
		if (ptr_raw_ecl_ins_hdr) memcpy(ptr_raw_ecl_ins_hdr, &raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr));
	}

	return size;
}

RawIns::RawIns(uint16_t id, uint8_t difficulty_mask, uint32_t post_pop_count, uint8_t param_count, uint16_t param_mask, const string& str_raw_params)
	: id(id), difficulty_mask(difficulty_mask), post_pop_count(post_pop_count), param_count(param_count), param_mask(param_mask), str_raw_params(str_raw_params) {}

RawIns::~RawIns() {}

size_t RawIns::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	size_t size = 0;

	struct {
		__pragma(pack(push, 1));
		uint32_t time;
		uint16_t id;
		uint16_t size;
		uint16_t param_mask;
		/* The difficulty bitmask.
		*   1111LHNE
		* Bits mean: easy, normal, hard, lunatic. The rest are always set to 1. */
		uint8_t diff_mask;
		/* There doesn't seem to be a way of telling how many parameters there are
		* from the additional data. */
		uint8_t param_count;
		/* From TH13 on, this field stores the number of current stack references
		* in the parameter list. */
		uint32_t post_pop_count;
		__pragma(pack(pop));
	} raw_ecl_ins_hdr;
	char* ptr_raw_ecl_ins_hdr = nullptr;
	size += sizeof(raw_ecl_ins_hdr);
	if (ptr && size_buf >= size) {
		raw_ecl_ins_hdr.time = 0;
		raw_ecl_ins_hdr.id = this->id;
		if (sizeof(raw_ecl_ins_hdr) + this->str_raw_params.size() > UINT16_MAX) throw(exception("Too large instruction."));
		raw_ecl_ins_hdr.size = (sizeof(raw_ecl_ins_hdr) + this->str_raw_params.size()) & ~(uint16_t)0;
		raw_ecl_ins_hdr.param_mask = this->param_mask;
		raw_ecl_ins_hdr.diff_mask = this->difficulty_mask;
		raw_ecl_ins_hdr.param_count = this->param_count;
		raw_ecl_ins_hdr.post_pop_count = this->post_pop_count;
		ptr_raw_ecl_ins_hdr = append_data(ptr, &raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr));
	}

	size += this->str_raw_params.size();
	if (ptr && size_buf >= size) {
		for (char val_param_byte : this->str_raw_params) {
			append_data(ptr, &val_param_byte, sizeof(char));
		}
	}

	return size;
}

fSub::fSub(const string& name, uint32_t count_var, const vector<shared_ptr<fSubDataEntry>>& data_entries)
	:name(name), count_var(count_var), data_entries(data_entries) {}

fRoot::fRoot(const vector<fSub>& subs, const vector<string>& ecli, const vector<string>& anim)
	: subs(subs), ecli(ecli), anim(anim) {}
