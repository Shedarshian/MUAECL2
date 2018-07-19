#include "stdafx.h"
#include "Misc.h"
#include "Misc2.h"
#include "RawEclGenerator.h"

using namespace std;

// Append size bytes of data from src to dst, and increase dst by size.
#define APPEND_DATA(dst, src, size) (reinterpret_cast<char*>(memcpy((*reinterpret_cast<char**>(&dst) += (size)) - (size), (src), (size))))
// Append some null bytes to align ptr and size to 4 bytes boundary.
#define ALIGN4_DATA(ptr, size_buf, size) if ((size) % 4) { uint32_t _dummy = 0; if ((ptr) && (size_buf) >= (size) + 4 - ((size) % 4)) APPEND_DATA((ptr), &_dummy, 4 - ((size) % 4)); (size) +=  4 - ((size) % 4); }

bool Parameter_int::is_float() const { return false; }

bool Parameter_int::is_ref_param() const { return false; }

int32_t Parameter_int::get_ref_id(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_int::get_ref_id")); }

size_t Parameter_int::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
	if (ptr && size_buf >= sizeof(int)) {
		APPEND_DATA(ptr, &this->val, sizeof(int));
	}
	return sizeof(int);
}

bool Parameter_float::is_float() const { return true; }

bool Parameter_float::is_ref_param() const { return false; }

int32_t Parameter_float::get_ref_id(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_float::get_ref_id")); }

size_t Parameter_float::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
	if (ptr && size_buf >= sizeof(float)) {
		APPEND_DATA(ptr, &this->val, sizeof(float));
	}
	return sizeof(float);
}

bool Parameter_variable::is_float() const { return this->isFloat; }

bool Parameter_variable::is_ref_param() const { return true; }

int32_t Parameter_variable::get_ref_id(const SubSerializationContext& sub_ctx) const {
	if (this->id_var >= sub_ctx.count_var) throw(ErrDesignApp("Parameter_variable::get_ref_id : id_var out of range"));
	if (this->id_var > INT32_MAX / 4) throw(exception("Too many local variables."));
	return this->id_var * 4;
}

size_t Parameter_variable::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	int32_t ref_id = INT32_MAX;
	if (ptr && size_buf >= sizeof(float)) {
		ref_id = this->get_ref_id(sub_ctx);
	}
	if (this->isFloat) {
		static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(float)) {
			// If ref_id is more than 24 bits, storing it in a single-precision floating point value would result in inaccuracy.
			if (ref_id & 0x7F000000) throw(exception("Too many variables."));
#pragma warning(push)
#pragma warning(disable:4244)
			float f_ref_id = ref_id;
#pragma warning(pop)
			APPEND_DATA(ptr, &f_ref_id, sizeof(float));
		}
		return sizeof(float);
	} else {
		static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(int)) {
			int i_ref_id = ref_id;
			APPEND_DATA(ptr, &i_ref_id, sizeof(int));
		}
		return sizeof(int);
	}
}

bool Parameter_stack::is_float() const { return this->isFloat; }

bool Parameter_stack::is_ref_param() const { return true; }

int32_t Parameter_stack::get_ref_id(const SubSerializationContext& sub_ctx) const {
	return this->id;
}

size_t Parameter_stack::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	int32_t ref_id = INT32_MAX;
	if (ptr && size_buf >= sizeof(float)) {
		ref_id = this->get_ref_id(sub_ctx);
	}
	if (this->isFloat) {
		static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(float)) {
			// If ref_id is more than 24 bits, storing it in a single-precision floating point value would result in inaccuracy.
			if (ref_id & 0x7F000000) throw(exception("Too large stack reference ID."));
#pragma warning(push)
#pragma warning(disable:4244)
			float f_ref_id = ref_id;
#pragma warning(pop)
			APPEND_DATA(ptr, &f_ref_id, sizeof(float));
		}
		return sizeof(float);
	} else {
		static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(int)) {
			int i_ref_id = ref_id;
			APPEND_DATA(ptr, &i_ref_id, sizeof(int));
		}
		return sizeof(int);
	}
}

bool Parameter_env::is_float() const { return this->isFloat; }

bool Parameter_env::is_ref_param() const { return true; }

int32_t Parameter_env::get_ref_id(const SubSerializationContext& sub_ctx) const {
	static_assert(sizeof(int) == sizeof(int32_t), "sizeof(int) is not equal to sizeof(int32_t).");
	return -this->id;
}

size_t Parameter_env::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	int32_t ref_id = INT32_MAX;
	if (ptr && size_buf >= sizeof(float)) {
		ref_id = this->get_ref_id(sub_ctx);
	}
	if (this->isFloat) {
		static_assert(sizeof(float) == sizeof(uint32_t), "sizeof(float) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(float)) {
			// If ref_id is more than 24 bits, storing it in a single-precision floating point value would result in inaccuracy.
			if (ref_id & 0x7F000000) throw(exception("Too large environment reference ID."));
#pragma warning(push)
#pragma warning(disable:4244)
			float f_ref_id = ref_id;
#pragma warning(pop)
			APPEND_DATA(ptr, &f_ref_id, sizeof(float));
		}
		return sizeof(float);
	} else {
		static_assert(sizeof(int) == sizeof(uint32_t), "sizeof(int) is not equal to sizeof(uint32_t).");
		if (ptr && size_buf >= sizeof(int)) {
			int i_ref_id = ref_id;
			APPEND_DATA(ptr, &i_ref_id, sizeof(int));
		}
		return sizeof(int);
	}
}

bool Parameter_jmp::is_float() const { return false; }

bool Parameter_jmp::is_ref_param() const { return false; }

int32_t Parameter_jmp::get_ref_id(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_jmp::get_ref_id")); }

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
		APPEND_DATA(ptr, &offs_jmp, sizeof(int32_t));
	}
	return sizeof(int32_t);
}

bool Parameter_string::is_float() const { return false; }

bool Parameter_string::is_ref_param() const { return false; }

int32_t Parameter_string::get_ref_id(const SubSerializationContext& sub_ctx) const { throw(ErrDesignApp("Parameter_string::get_ref_id")); }

size_t Parameter_string::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	size_t size = 0;
	if (this->str.size() > UINT32_MAX - 4) throw(exception("String parameter too large."));
	uint32_t size_strdata = (this->str.size() & ~(uint32_t)0) / 4 * 4 + 4;
	size += sizeof(uint32_t);
	if (ptr && size_buf >= size) {
		APPEND_DATA(ptr, &size_strdata, sizeof(uint32_t));
	}
	size += size_strdata * sizeof(char);
	if (ptr && size_buf >= size) {
		unique_ptr<char[]> strdata(new char[size_strdata]());
		memcpy(strdata.get(), this->str.data(), this->str.size() * sizeof(char));
		APPEND_DATA(ptr, strdata.get(), size_strdata * sizeof(char));
	}
	return size;
}

bool Parameter_call::is_float() const { return this->isToFloat; }

bool Parameter_call::is_ref_param() const { return this->param->is_ref_param(); }

int32_t Parameter_call::get_ref_id(const SubSerializationContext& sub_ctx) const { return this->param->get_ref_id(sub_ctx); }

size_t Parameter_call::serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const {
	size_t size = 0;
	size += sizeof(uint32_t);
	if (ptr && size_buf >= size) {
		uint32_t typeval = (this->isFromFloat ? 0x66 : 0x69) | (this->isToFloat ? 0x6600 : 0x6900);
		APPEND_DATA(ptr, &typeval, sizeof(uint32_t));
	}
	if (this->isFromFloat != this->param->is_float()) throw(ErrDesignApp("Parameter_call::serialize : actual parameter type mismatch"));
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

Ins::Ins(int id, const vector<Parameter*>& paras, uint8_t difficulty_mask, int time)
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
		uint32_t cur_stack_ref_count;
		__pragma(pack(pop));
	} raw_ecl_ins_hdr;
	char* ptr_raw_ecl_ins_hdr = nullptr;
	size += sizeof(raw_ecl_ins_hdr);
	if (ptr && size_buf >= size) {
		if (this->time < 0 || this->time > UINT32_MAX) throw(exception("Invalid instruction time."));
		raw_ecl_ins_hdr.time = this->time;
		if (this->id < 0 || this->id > UINT16_MAX) throw(exception("Invalid instruction ID."));
		raw_ecl_ins_hdr.id = this->id;
		//raw_ecl_ins_hdr.size;
		raw_ecl_ins_hdr.param_mask = 0;
		raw_ecl_ins_hdr.diff_mask = this->difficulty_mask;
		//raw_ecl_ins_hdr.param_count;
		//raw_ecl_ins_hdr.cur_stack_ref_count;
		ptr_raw_ecl_ins_hdr = APPEND_DATA(ptr, &raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr));
	}

	std::vector<Parameter*> vec_param(this->paras);
	if (vec_param.size() > 16) throw(exception("Too many parameters."));
	if (ptr && size_buf >= size) {
		raw_ecl_ins_hdr.param_count = vec_param.size() & ~(uint8_t)0;
	}
	int i = 0;
	for (const Parameter* val_param : vec_param) {
		size_t size_param = val_param->serialize(nullptr, 0, sub_ctx);
		size += size_param;
		if (ptr && size_buf >= size) {
			if (val_param->is_ref_param()) {
				raw_ecl_ins_hdr.param_mask |= 1 << i;
				int32_t ref_id = val_param->get_ref_id(sub_ctx);
				if (raw_ecl_ins_hdr.cur_stack_ref_count > ((uint32_t)-INT32_MIN) - 1) throw(exception("Too large stack reference ID."));
				if (ref_id < 0 && -ref_id == raw_ecl_ins_hdr.cur_stack_ref_count + 1) ++raw_ecl_ins_hdr.cur_stack_ref_count;
			}
			if (val_param->serialize((ptr += size_param) - size_param, size_param, sub_ctx) != size_param) throw(ErrDesignApp("Inconsistent returned size when calling Parameter::serialize"));
		}
		++i;
	}
	raw_ecl_ins_hdr.cur_stack_ref_count <<= 3;

	if (ptr && size_buf >= size) {
		if (size > UINT16_MAX) throw(exception("Too large instruction."));
		raw_ecl_ins_hdr.size = size & ~(uint16_t)0;
		if (ptr_raw_ecl_ins_hdr) memcpy(ptr_raw_ecl_ins_hdr, &raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr));
	}

	return size;
}

fSub::fSub(const string& name, uint32_t count_var, const vector<shared_ptr<fSubDataEntry>>& data_entries)
	:name(name), count_var(count_var), data_entries(data_entries) {}

fRoot::fRoot(const vector<fSub>& subs, const vector<string>& ecli, const vector<string>& anim)
	: subs(subs), ecli(ecli), anim(anim) {}
