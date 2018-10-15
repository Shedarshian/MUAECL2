#include "stdafx.h"
#include <climits>
#include <memory>
#include <algorithm>
#include <random>
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

RawEclGenerator::RawEclGenerator(const fRoot& root)
	: root(root) {}

RawEclGenerator::~RawEclGenerator() {}

void RawEclGenerator::generate(std::ostream& stream) const {
	size_t size = this->generate(nullptr, 0);
	unique_ptr<char[]> ptr(new char[size ? size : 1]());
	if (this->generate(ptr.get(), size) != size) throw(ErrDesignApp("Inconsistent returned size when calling RawEclGenerator::generate"));
	stream.write(ptr.get(), size);
}

size_t RawEclGenerator::generate(char* ptr, size_t size_buf) const {
	size_t size = 0;

	struct {
		__pragma(pack(push, 1));
		char magic[4] = { 'S', 'C', 'P', 'T' };
		uint16_t unknown1 = 1;
		uint16_t include_length;
		uint32_t include_offset;
		uint32_t zero1 = 0;
		uint32_t sub_count;
		uint32_t zero2[4] = { 0, 0, 0, 0 };
		__pragma(pack(pop));
	} raw_ecl_file_hdr;
	char* ptr_raw_ecl_file_hdr = nullptr;
	size += sizeof(raw_ecl_file_hdr);
	if (ptr && size_buf >= size) {
		//raw_ecl_file_hdr.include_length;
		//raw_ecl_file_hdr.include_offset;
		//raw_ecl_file_hdr.sub_count;
		ptr_raw_ecl_file_hdr = append_data(ptr, &raw_ecl_file_hdr, sizeof(raw_ecl_file_hdr));
	}
	align4_data(ptr, size_buf, size);

	if (ptr && size_buf >= size) {
		if (size > UINT32_MAX) throw(exception("Output file too large."));
		raw_ecl_file_hdr.include_offset = size & ~(uint32_t)0;
	}
	size_t size_raw_includes = this->make_raw_includes(nullptr, 0);
	size += size_raw_includes;
	if (ptr && size_buf >= size) {
		if (this->make_raw_includes((ptr += size_raw_includes) - size_raw_includes, size_raw_includes) != size_raw_includes) throw(ErrDesignApp("Inconsistent returned size when calling RawEclGenerator::make_raw_includes"));
	}
	align4_data(ptr, size_buf, size);
	if (ptr && size_buf >= size) {
		if (size - raw_ecl_file_hdr.include_offset > UINT16_MAX) throw(exception("Too many includes."));
		raw_ecl_file_hdr.include_length = (size - raw_ecl_file_hdr.include_offset) & ~(uint16_t)0;
	}

	vector<fSub> vec_sub(this->root.subs);
	sort(vec_sub.begin(), vec_sub.end(),
		[](const fSub& l, const fSub& r)->bool {
			return l.name < r.name;
		}
	);

	if (ptr && size_buf >= size) {
		if (vec_sub.size() > UINT32_MAX) throw(exception("Too many subroutines."));
		raw_ecl_file_hdr.sub_count = vec_sub.size() & ~(uint32_t)0;
		memcpy(ptr_raw_ecl_file_hdr, &raw_ecl_file_hdr, sizeof(raw_ecl_file_hdr));
	}

	uint32_t* ptr_raw_ecl_sub_offsets = nullptr;
	size += vec_sub.size() * sizeof(uint32_t);
	if (ptr && size_buf >= size) {
		ptr_raw_ecl_sub_offsets = reinterpret_cast<uint32_t*>((ptr += vec_sub.size() * sizeof(uint32_t)) - vec_sub.size() * sizeof(uint32_t));
	}

	for (const fSub& val_sub : vec_sub) {
		size += val_sub.name.size() + 1;
		if (ptr && size_buf >= size) {
			append_data(ptr, val_sub.name.c_str(), val_sub.name.size() + 1);
		}
	}
	align4_data(ptr, size_buf, size);

	for (const fSub& val_sub : vec_sub) {
		if (size > UINT32_MAX) throw(exception("Too large generated raw ECL file."));
		if (ptr_raw_ecl_sub_offsets) *(ptr_raw_ecl_sub_offsets++) = size & ~(uint32_t)0;
		size_t size_raw_sub = this->make_raw_sub(nullptr, 0, val_sub);
		size += size_raw_sub;
		if (ptr && size_buf >= size) {
			if (this->make_raw_sub((ptr += size_raw_sub) - size_raw_sub, size_raw_sub, val_sub) != size_raw_sub) throw(ErrDesignApp("Inconsistent returned size when calling RawEclGenerator::make_raw_sub"));
		}
	}
	return size;
}

size_t RawEclGenerator::make_raw_includes(char* ptr, size_t size_buf) const {
	size_t size = 0;

	struct {
		__pragma(pack(push, 1));
		char magic[4] = { 'A', 'N', 'I', 'M' };
		uint32_t count = 0;
		__pragma(pack(pop));
	} anim_hdr;
	vector<string> vec_anim(this->root.anim);
	size += sizeof(anim_hdr);
	if (ptr && size_buf >= size) {
		if (vec_anim.size() > UINT32_MAX) throw(exception("Too many ANIM includes."));
		anim_hdr.count = vec_anim.size() & ~(uint32_t)0;
		append_data(ptr, &anim_hdr, sizeof(anim_hdr));
	}
	for (const string& val_anim : vec_anim) {
		size += val_anim.size() + 1;
		if (ptr && size_buf >= size) {
			append_data(ptr, val_anim.c_str(), val_anim.size() + 1);
		}
	}
	align4_data(ptr, size_buf, size);

	struct {
		__pragma(pack(push, 1));
		char magic[4] = { 'E', 'C', 'L', 'I' };
		uint32_t count = 0;
		__pragma(pack(pop));
	} ecli_hdr;
	vector<string> vec_ecli(this->root.ecli);
	size += sizeof(ecli_hdr);
	if (ptr && size_buf >= size) {
		if (vec_ecli.size() > UINT32_MAX) throw(exception("Too many ECLI includes."));
		ecli_hdr.count = vec_ecli.size() & ~(uint32_t)0;
		append_data(ptr, &ecli_hdr, sizeof(ecli_hdr));
	}
	for (const string& val_ecli : vec_ecli) {
		size += val_ecli.size() + 1;
		if (ptr && size_buf >= size) {
			append_data(ptr, val_ecli.c_str(), val_ecli.size() + 1);
		}
	}
	align4_data(ptr, size_buf, size);

	return size;
}

size_t RawEclGenerator::make_raw_sub(char* ptr, size_t size_buf, const fSub& sub) const {
	size_t size = 0;

	struct {
		__pragma(pack(push, 1));
		char magic[4] = { 'E', 'C', 'L', 'H' };
		uint32_t data_offset;
		uint32_t zero[2] = { 0, 0 };
		__pragma(pack(pop));
	} raw_ecl_sub_hdr;
	char* ptr_raw_ecl_sub_hdr = nullptr;
	size += sizeof(raw_ecl_sub_hdr);
	if (ptr && size_buf >= size) {
		//raw_ecl_sub_hdr.data_offset;
		ptr_raw_ecl_sub_hdr = append_data(ptr, &raw_ecl_sub_hdr, sizeof(raw_ecl_sub_hdr));
	}

	if (ptr && size_buf >= size) {
		if (size > UINT32_MAX) throw(exception("Output file too large."));
		raw_ecl_sub_hdr.data_offset = size & ~(uint32_t)0;
		memcpy(ptr_raw_ecl_sub_hdr, &raw_ecl_sub_hdr, sizeof(raw_ecl_sub_hdr));
	}

	SubSerializationContext ctx(sub.count_var, sub.data_entries);

	size += ctx.vec_offs_data_entry[ctx.vec_data_entry.size()] - ctx.vec_offs_data_entry[0];
	if (ptr && size_buf >= size) {
		ctx.i_data_entry_current = 0;
		for (const shared_ptr<fSubDataEntry>& val_data_entry : ctx.vec_data_entry) {
			size_t size_data_entry = ctx.vec_offs_data_entry[ctx.i_data_entry_current + 1] - ctx.vec_offs_data_entry[ctx.i_data_entry_current];
			if (val_data_entry->serialize((ptr += size_data_entry) - size_data_entry, size_data_entry, ctx) != size_data_entry) throw(ErrDesignApp("Inconsistent returned size when calling fSubDataEntry::serialize"));
			++ctx.i_data_entry_current;
		}
	}

	return size;
}

SubSerializationContext::SubSerializationContext(uint32_t count_var, const vector<shared_ptr<fSubDataEntry>>& data_entries)
	:count_var(count_var) {
	if (count_var > INT32_MAX / 4) throw(exception("Too many local variables."));
	mt19937 randengine;
	{
		vector<uint_least32_t> vec_seed;
		for (const shared_ptr<fSubDataEntry>& val_data_entry : data_entries) {
			std::shared_ptr<Ins> ins = dynamic_pointer_cast<Ins, fSubDataEntry>(val_data_entry);
			if (ins) vec_seed.push_back(ins->id);
		}
		seed_seq seedseq(vec_seed.cbegin(), vec_seed.cend());
		randengine.seed(seedseq);
	}
	this->vec_data_entry.emplace_back(new Ins(40, vector<Parameter*>({ new Parameter_int(count_var * 4) })));
	this->vec_data_entry.insert(this->vec_data_entry.cend(), data_entries.cbegin(), data_entries.cend());
	this->vec_data_entry.emplace_back(new Ins(41, vector<Parameter*>()));
	this->vec_data_entry.emplace_back(new Ins(10, vector<Parameter*>()));
	{
		vector<Parameter*> vec_param_dummy;
		unsigned int count_param_dummy = randengine() % 4;
		for (unsigned int i = 0; i < count_param_dummy; ++i) {
			vec_param_dummy.push_back((randengine() & 1) ? static_cast<Parameter*>(new Parameter_float((float)0.261799388 * (randengine() % 24))) : static_cast<Parameter*>(new Parameter_int(randengine() % 0x1000)));
		}
		this->vec_data_entry.emplace_back(new Ins((uint16_t)-1919, vec_param_dummy));
	}
	this->vec_offs_data_entry.resize(this->vec_data_entry.size() + 1);
	this->vec_offs_data_entry[0] = 0;
	this->i_data_entry_current = 0;
	for (const shared_ptr<fSubDataEntry>& val_data_entry : this->vec_data_entry) {
		this->vec_offs_data_entry[i_data_entry_current + 1] = this->vec_offs_data_entry[i_data_entry_current] + val_data_entry->serialize(nullptr, 0, *this);
		val_data_entry->set_offs(*this, this->vec_offs_data_entry[i_data_entry_current]);
		++this->i_data_entry_current;
	}
}

SubSerializationContext::~SubSerializationContext() {}
