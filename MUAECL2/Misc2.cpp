#include "stdafx.h"
#include "Misc.h"
#include "Misc2.h"

using namespace std;

// Append size bytes of data from src to dst, and increase dst by size.
#define APPEND_DATA(dst, src, size) (reinterpret_cast<char*>(memcpy((*reinterpret_cast<char**>(&dst) += (size)) - (size), (src), (size))))
// Append some null bytes to align ptr and size to 4 bytes boundary.
#define ALIGN4_DATA(ptr, size_buf, size) if ((size) % 4) { uint32_t _dummy = 0; if ((ptr) && (size_buf) >= (size) + 4 - ((size) % 4)) APPEND_DATA((ptr), &_dummy, 4 - ((size) % 4)); (size) +=  4 - ((size) % 4); }

Ins::Ins(int id, const vector<Parameter*>& paras, bool E, bool N, bool H, bool L, int time)
	:id(id), paras(paras), diff { E, N, H, L }, time(time) {}

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
		raw_ecl_ins_hdr.diff_mask = 0xF0;
		for (int i = 0; i < 4; ++i) if (this->diff[i]) raw_ecl_ins_hdr.diff_mask |= 1 << i;
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
	for (auto val_param : vec_param) {
		size_t size_param = val_param->serialize(nullptr, 0, sub_ctx);
		size += size_param;
		if (ptr && size_buf >= size) {
			if (val_param->is_stack_param()) {
				raw_ecl_ins_hdr.param_mask |= 1 << i;
				int32_t stack_id = val_param->get_stack_id(sub_ctx);
				if (raw_ecl_ins_hdr.cur_stack_ref_count > ((uint32_t)-INT32_MIN) - 1) throw(exception("Too large stack reference ID."));
				if (stack_id < 0 && -stack_id == raw_ecl_ins_hdr.cur_stack_ref_count + 1) ++raw_ecl_ins_hdr.cur_stack_ref_count;
			}
			if (val_param->serialize((ptr += size_param) - size_param, size_param, sub_ctx) != size_param) throw(ErrDesignApp("Inconsistent returned size when calling Parameter::serialize"));
		}
		++i;
	}

	if (ptr && size_buf >= size) {
		if (size > UINT16_MAX) throw(exception("Too large instruction."));
		raw_ecl_ins_hdr.size = size & ~(uint16_t)0;
		memcpy(ptr_raw_ecl_ins_hdr, &raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr));
	}

	return size;
}