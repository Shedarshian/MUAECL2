#include "stdafx.h"
#include <climits>
#include <memory>
#include "RawEclGenerator.h"

using namespace std;

// Append size bytes of data from src to dst, and increase dst by size.
#define APPEND_DATA(dst, src, size) (reinterpret_cast<char*>(memcpy((reinterpret_cast<char*>(dst) += (size)) - (size), (src), (size))))
// Append some null bytes to align ptr and size to 4 bytes boundary.
#define ALIGN4_DATA(ptr, size_buf, size) if ((size) % 4) { uint32_t _dummy = 0; if ((ptr) && (size_buf) >= (size) + 4 - ((size) % 4)) APPEND_DATA((ptr), &_dummy, 4 - ((size) % 4)); (size) +=  4 - ((size) % 4); }

RawEclGenerator::RawEclGenerator(const fRoot& root)
	:root(root) {}

RawEclGenerator::~RawEclGenerator() {}

void RawEclGenerator::generate(std::ostream& stream) const {
	size_t size = this->generate(nullptr, 0);
	unique_ptr<char[]> ptr(new char[size]());
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
		ptr_raw_ecl_file_hdr = APPEND_DATA(ptr, &raw_ecl_file_hdr, sizeof(raw_ecl_file_hdr));
	}
	ALIGN4_DATA(ptr, size_buf, size);

	if (ptr && size_buf >= size) {
		raw_ecl_file_hdr.include_offset = size;
	}
	size_t size_raw_includes = this->make_raw_includes(nullptr, 0);
	size += size_raw_includes;
	if (ptr && size_buf >= size) {
		if (this->make_raw_includes((ptr += size_raw_includes) - size_raw_includes, size_raw_includes) != size_raw_includes) throw(ErrDesignApp("Inconsistent returned size when calling RawEclGenerator::make_raw_includes"));
	}
	ALIGN4_DATA(ptr, size_buf, size);
	if (ptr && size_buf >= size) {
		raw_ecl_file_hdr.include_length = size - raw_ecl_file_hdr.include_offset;
	}

	vector<fSub> vec_sub(this->root.subs);
	sort(vec_sub.begin(), vec_sub.end(),
		[](const fSub& l, const fSub& r)->bool {
			return l.name < r.name;
		}
	);

	if (ptr && size_buf >= size) {
		raw_ecl_file_hdr.sub_count = vec_sub.size();
		memcpy(ptr_raw_ecl_file_hdr, &raw_ecl_file_hdr, sizeof(raw_ecl_file_hdr));
	}

	uint32_t* ptr_raw_ecl_sub_offsets = nullptr;
	size += vec_sub.size() * sizeof(uint32_t);
	if (ptr && size_buf >= size) {
		ptr_raw_ecl_sub_offsets = reinterpret_cast<uint32_t*>((ptr += vec_sub.size() * sizeof(uint32_t)) - vec_sub.size() * sizeof(uint32_t));
	}

	for (fSub val_sub : vec_sub) {
		size += val_sub.name.size() + 1;
		if (ptr && size_buf >= size) {
			APPEND_DATA(ptr, val_sub.name.c_str(), val_sub.name.size() + 1);
		}
	}
	ALIGN4_DATA(ptr, size_buf, size);

	for (fSub val_sub : vec_sub) {
		*(ptr_raw_ecl_sub_offsets++) = size;
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
		uint32_t count;
		__pragma(pack(pop));
	} anim_hdr;
	vector<string> vec_anim(this->root.anim);
	size += sizeof(anim_hdr);
	if (ptr && size_buf >= size) {
		if (vec_anim.size() > UINT32_MAX) throw(exception("Too many ANIM includes."));
		anim_hdr.count = vec_anim.size() & (~(uint32_t)0);
		APPEND_DATA(ptr, &anim_hdr, sizeof(anim_hdr));
	}
	for (string val_anim : vec_anim) {
		size += val_anim.size() + 1;
		if (ptr && size_buf >= size) {
			APPEND_DATA(ptr, val_anim.c_str(), val_anim.size() + 1);
		}
	}
	ALIGN4_DATA(ptr, size_buf, size);

	struct {
		__pragma(pack(push, 1));
		char magic[4] = { 'E', 'C', 'L', 'I' };
		uint32_t count;
		__pragma(pack(pop));
	} ecli_hdr;
	vector<string> vec_ecli(this->root.ecli);
	size += sizeof(ecli_hdr);
	if (ptr && size_buf >= size) {
		if (vec_ecli.size() > UINT32_MAX) throw(exception("Too many ECLI includes."));
		ecli_hdr.count = vec_ecli.size() & (~(uint32_t)0);
		APPEND_DATA(ptr, &ecli_hdr, sizeof(ecli_hdr));
	}
	for (string val_ecli : vec_ecli) {
		size += val_ecli.size() + 1;
		if (ptr && size_buf >= size) {
			APPEND_DATA(ptr, val_ecli.c_str(), val_ecli.size() + 1);
		}
	}
	ALIGN4_DATA(ptr, size_buf, size);

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
		ptr_raw_ecl_sub_hdr = APPEND_DATA(ptr, &raw_ecl_sub_hdr, sizeof(raw_ecl_sub_hdr));
	}
	// TODO: Implement RawEclGenerator::make_raw_sub.
}
