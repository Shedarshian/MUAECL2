#include "stdafx.h"
#include <cstdint>
#include <climits>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <algorithm>
#include <utility>
#include "RawEclDecoder.h"
#include "Misc.h"

using namespace std;

static inline void seek_delta(ptrdiff_t diff, const char*& ptr, size_t& size_buf, const char* ptr_initial) {
	if (diff > 0 && size_buf < (size_t)diff) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
	ptr += diff;
	size_buf -= diff;
}

static inline void seek_to(size_t offs_target, const char*& ptr, size_t& size_buf, const char* ptr_initial) {
	ptrdiff_t diff = ptr_initial + offs_target - ptr;
	if (diff > 0 && size_buf < (size_t)diff) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
	ptr += diff;
	size_buf -= diff;
}

static inline void extract_data(void* dst, size_t size, const char*& ptr, size_t& size_buf, const char* ptr_initial) {
	if (size_buf < size) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
	memcpy(dst, ptr, size);
	seek_delta(size, ptr, size_buf, ptr_initial);
}

static inline void seek_align4(const char*& ptr, size_t& size_buf, const char* ptr_initial) {
	ptrdiff_t offs = ptr - ptr_initial;
	if (offs % 4) {
		seek_delta(4 - (offs % 4), ptr, size_buf, ptr_initial);
	}
}

DecodedParam::DecodedParam(ParamType param_type) : param_type(param_type) {}

DecodedParam::~DecodedParam() {}

DecodedParam_Int::DecodedParam_Int(int32_t val) : DecodedParam(DecodedParam::ParamType::Int), val(val) {}

DecodedParam_Int::~DecodedParam_Int() {}

DecodedParam_Float::DecodedParam_Float(float val) : DecodedParam(DecodedParam::ParamType::Float), val(val) {}

DecodedParam_Float::~DecodedParam_Float() {}

DecodedParam_String::DecodedParam_String(string str) : DecodedParam(DecodedParam::ParamType::String), val(val) {}

DecodedParam_String::~DecodedParam_String() {}

DecodedParam_Variable::DecodedParam_Variable(uint32_t id_var, bool is_float) : DecodedParam(DecodedParam::ParamType::Variable), id_var(id_var), is_float(is_float) {}

DecodedParam_Variable::~DecodedParam_Variable() {}

DecodedParam_AbnormalVariable::DecodedParam_AbnormalVariable(int32_t ref_id, bool is_float) : DecodedParam(DecodedParam::ParamType::AbnormalVariable), ref_id(ref_id), is_float(is_float) {}

DecodedParam_AbnormalVariable::~DecodedParam_AbnormalVariable() {}

DecodedParam_Stack::DecodedParam_Stack(int32_t ref_id, bool is_float) : DecodedParam(DecodedParam::ParamType::Stack), ref_id(ref_id), is_float(is_float) {}

DecodedParam_Stack::~DecodedParam_Stack() {}

DecodedParam_Env::DecodedParam_Env(uint32_t env_id, bool is_float) : DecodedParam(DecodedParam::ParamType::Env), env_id(env_id), is_float(is_float) {}

DecodedParam_Env::~DecodedParam_Env() {}

DecodedParam_Jmp::DecodedParam_Jmp(uint32_t id_target) : DecodedParam(DecodedParam::ParamType::Jmp), id_target(id_target) {}

DecodedParam_Jmp::~DecodedParam_Jmp() {}

DecodedParam_Call::DecodedParam_Call(shared_ptr<DecodedParam>& param, bool is_from_float, bool is_to_float) : DecodedParam(DecodedParam::ParamType::Call), param(param), is_from_float(is_from_float), is_to_float(is_to_float) {}

DecodedParam_Call::~DecodedParam_Call() {}

DecodedJmpTarget::DecodedJmpTarget(uint32_t id_target) : id_target(id_target) {}

DecodedJmpTarget::~DecodedJmpTarget() {}

DecodedIns::DecodedIns() {}

DecodedIns::DecodedIns(const DecodedIns& t) {
	this->time = t.time;
	this->id = t.id;
	this->difficulty_mask = t.difficulty_mask;
	this->post_pop_count = t.post_pop_count;
	this->is_rawins = t.is_rawins;
	if (t.is_rawins) {
		this->rawins_params = t.rawins_params ? new rawins_params_t(*t.rawins_params) : nullptr;
	} else {
		this->params = t.params ? new vector<shared_ptr<DecodedParam>>(*t.params) : nullptr;
	}
}

DecodedIns::DecodedIns(DecodedIns&& t) {
	this->time = t.time;
	this->id = t.id;
	this->difficulty_mask = t.difficulty_mask;
	this->post_pop_count = t.post_pop_count;
	this->is_rawins = t.is_rawins;
	if (t.is_rawins) {
		this->rawins_params = t.rawins_params;
		t.rawins_params = nullptr;
	} else {
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
	} else {
		if (this->params) {
			delete this->params;
			this->params = nullptr;
		}
	}
}

RawEclDecoder::RawEclDecoder() {
	for (const pair<string, pair<int, vector<ReadIns::NumType>>>& val_readins_ins : ReadIns::ins) {
		this->map_ins.emplace(
			make_pair(
				val_readins_ins.second.first,
				ins_def_t(
					val_readins_ins.second.first,
					val_readins_ins.first,
					val_readins_ins.second.second
				)
			)
		);
	}
}

RawEclDecoder::~RawEclDecoder() {}

shared_ptr<DecodedRoot> RawEclDecoder::DecodeRawEclRoot(istream& stream) const {
	stream.seekg(0, std::ios_base::end);
	intptr_t length = stream.tellg();
	stream.seekg(0, std::ios_base::beg);
	if (length < 0) throw(exception("RawEclDecoder::DecodeRawEclRoot : failed to get input stream size"));
	unique_ptr<char[]> buf(new char[length]());
	stream.read(buf.get(), length);
	return this->DecodeRawEclRoot(buf.get(), length);
}

RawEclDecoder::ins_def_t::ins_def_t(int id, const string& name, const vector<ReadIns::NumType>& params)
	: id(id), name(name), params(params) {}

RawEclDecoder::ins_def_t::~ins_def_t() {}

shared_ptr<DecodedRoot> RawEclDecoder::DecodeRawEclRoot(const char* ptr, size_t size_buf) const {
	shared_ptr<DecodedRoot> root(new DecodedRoot());
	const char* const ptr_initial = ptr;

	struct {
		__pragma(pack(push, 1));
		char magic[4];
		uint16_t unknown1;
		uint16_t include_length;
		uint32_t include_offset;
		uint32_t zero1;
		uint32_t sub_count;
		uint32_t zero2[4];
		__pragma(pack(pop));
	} raw_ecl_file_hdr;
	extract_data(&raw_ecl_file_hdr, sizeof(raw_ecl_file_hdr), ptr, size_buf, ptr_initial);
	if (strncmp(raw_ecl_file_hdr.magic, "SCPT", sizeof(raw_ecl_file_hdr.magic))) throw(ErrDecoderInvalidEcl(0));

	seek_to(raw_ecl_file_hdr.include_offset, ptr, size_buf, ptr_initial);
	if (size_buf < raw_ecl_file_hdr.include_length) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
	try {
		try {
			this->DecodeRawEclIncludes(ptr, raw_ecl_file_hdr.include_length, root);
		} catch (ErrDecoderUnexpEof& err) {
			throw(ErrDecoderInvalidIncludes(err.GetOffset()));
		}
	} catch (DecoderException& err) {
		err.AdjustOffset(ptr_initial, ptr);
		throw;
	}
	seek_delta(raw_ecl_file_hdr.include_length, ptr, size_buf, ptr_initial);

	vector<uint32_t> vec_sub_offsets(raw_ecl_file_hdr.sub_count);
	for (uint32_t i_sub = 0; i_sub < raw_ecl_file_hdr.sub_count; ++i_sub) {
		extract_data(&vec_sub_offsets.at(i_sub), sizeof(uint32_t), ptr, size_buf, ptr_initial);
	}

	vector<string> vec_sub_names(raw_ecl_file_hdr.sub_count);
	for (uint32_t i_sub = 0; i_sub < raw_ecl_file_hdr.sub_count; ++i_sub) {
		size_t len_str = strnlen(ptr, size_buf);
		if (len_str < 0) throw(ErrDecoderInvalidEcl(ptr - ptr_initial));
		if (size_buf < len_str + 1) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
		vec_sub_names.at(i_sub) = string(ptr, len_str);
		seek_delta(len_str + 1, ptr, size_buf, ptr_initial);
	}
	seek_align4(ptr, size_buf, ptr_initial);

	root->subs.resize(raw_ecl_file_hdr.sub_count);
	for (uint32_t i_sub = 0; i_sub < raw_ecl_file_hdr.sub_count; ++i_sub) {
		seek_to(vec_sub_offsets.at(i_sub), ptr, size_buf, ptr_initial);
		try {
			try {
				this->DecodeRawEclSub(ptr, size_buf, root->subs.at(i_sub));
			} catch (ErrDecoderUnexpEof& err) {
				throw(ErrDecoderInvalidSub(err.GetOffset()));
			}
		} catch (DecoderException& err) {
			err.AdjustOffset(ptr_initial, ptr);
			throw;
		}

	}

	return root;
}

void RawEclDecoder::DecodeRawEclIncludes(const char* ptr, size_t size_buf, const shared_ptr<DecodedRoot>& root) const {
	const char* const ptr_initial = ptr;
	while (size_buf) {
		struct raw_include_hdr_t {
			__pragma(pack(push, 1));
			char magic[4];
			uint32_t count;
			__pragma(pack(pop));
		} raw_include_hdr;
		extract_data(&raw_include_hdr, sizeof(raw_include_hdr), ptr, size_buf, ptr_initial);
		for (uint32_t i = 0; i < raw_include_hdr.count; ++i) {
			size_t len_str = strnlen(ptr, size_buf);
			if (len_str < 0) throw(ErrDecoderInvalidIncludes(ptr - ptr_initial));
			if (size_buf < len_str + 1) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
			if (!strncmp(raw_include_hdr.magic, "ANIM", sizeof(raw_include_hdr.magic))) {
				root->anim.emplace_back(ptr, len_str);
			} else if (!strncmp(raw_include_hdr.magic, "ECLI", sizeof(raw_include_hdr.magic))) {
				root->ecli.emplace_back(ptr, len_str);
			}
			seek_delta(len_str + 1, ptr, size_buf, ptr_initial);
			seek_align4(ptr, size_buf, ptr_initial);
		}
	}
}

void RawEclDecoder::DecodeRawEclSub(const char* ptr, size_t size_buf, shared_ptr<DecodedSub>& sub) const {
	sub.reset(new DecodedSub());
	const char* const ptr_initial = ptr;
	struct {
		__pragma(pack(push, 1));
		char magic[4];
		uint32_t data_offset;
		uint32_t zero[2];
		__pragma(pack(pop));
	} raw_ecl_sub_hdr;
	extract_data(&raw_ecl_sub_hdr, sizeof(raw_ecl_sub_hdr), ptr, size_buf, ptr_initial);
	if (strncmp(raw_ecl_sub_hdr.magic, "ECLH", sizeof(raw_ecl_sub_hdr.magic))) throw(ErrDecoderInvalidSub(0));
	list<shared_ptr<DecodedSubDataEntry>> list_data_entry;
	/// <summary>The offset of the current instruction</summary>
	size_t offs_ins = 0;
	/// <summary>A map from the offset of a data entry to the iterator to the data entry.</summary>
	map<size_t, list<shared_ptr<DecodedSubDataEntry>>::const_iterator> map_it_list_data_entry_offs;
	/// <summary>A vector of the <c>DecodedParam_Jmp</c> parameters whose target IDs are to be set.</summary>
	vector<pair<DecodedParam_Jmp*, size_t>> vec_param_jmp_pending;
	while (size_buf) {
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
		extract_data(&raw_ecl_ins_hdr, sizeof(raw_ecl_ins_hdr), ptr, size_buf, ptr_initial);
		DecodedIns ins;
		ins.time = raw_ecl_ins_hdr.time;
		ins.id = raw_ecl_ins_hdr.id;
		ins.difficulty_mask = raw_ecl_ins_hdr.diff_mask;
		ins.post_pop_count = raw_ecl_ins_hdr.post_pop_count;
		if (size_buf < raw_ecl_ins_hdr.size - sizeof(raw_ecl_ins_hdr)) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
		try {
			try {
				// Parameters processing routine.
				[this, &raw_ecl_ins_hdr, &ins, offs_ins, &vec_param_jmp_pending](const char* ptr, size_t size_buf) {
					const char* const ptr_initial = ptr;
					if (ins.id == 12 || ins.id == 13 || ins.id == 14) {
						// For goto instructions:
						// Seek to the beginning of the instruction actual parameter list.
						seek_to(0, ptr, size_buf, ptr_initial);
						// Clear the instruction's parameter list.
						if (ins.is_rawins) {
							if (ins.rawins_params) {
								delete ins.rawins_params;
								ins.rawins_params = nullptr;
							}
						} else {
							if (ins.params) {
								delete ins.params;
								ins.params = nullptr;
							}
						}
						ins.is_rawins = false;
						ins.params = new vector<shared_ptr<DecodedParam>>();
						int32_t offs_jmp;
						extract_data(&offs_jmp, sizeof(int32_t), ptr, size_buf, ptr_initial);
						// The target ID of this parameter has not been set to a correct one yet.
						DecodedParam_Jmp* param_jmp = new DecodedParam_Jmp(UINT32_MAX);
						ins.params->emplace_back(param_jmp);
						vec_param_jmp_pending.emplace_back(param_jmp, offs_ins + offs_jmp);
						int32_t rawval_time;
						extract_data(&rawval_time, sizeof(int32_t), ptr, size_buf, ptr_initial);
						if (raw_ecl_ins_hdr.param_mask & (1 << 1)) {
							int32_t ref_id = rawval_time;
							if (ref_id > 0) {
								if (ref_id & 0x3) {
									ins.params->emplace_back(new DecodedParam_AbnormalVariable(ref_id, false));
								} else {
									ins.params->emplace_back(new DecodedParam_Variable((uint32_t)ref_id >> 2, false));
								}
							} else if (ref_id < 0 && ref_id >= -100) {
								ins.params->emplace_back(new DecodedParam_Stack(ref_id, false));
							} else if (ref_id < -100) {
								ins.params->emplace_back(new DecodedParam_Env(-ref_id, false));
							} else {
								throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(int32_t)));
							}
						} else {
							ins.params->emplace_back(new DecodedParam_Int(rawval_time));
						}
						return;
					}
					pair<multimap<int, ins_def_t>::const_iterator, multimap<int, ins_def_t>::const_iterator> range_map_ins(this->map_ins.equal_range(ins.id));
					multimap<int, ins_def_t>::const_iterator it_map_ins;
					for (it_map_ins = range_map_ins.first; it_map_ins != range_map_ins.second; ++it_map_ins) {
						// For each instruction slot whose ID is equal to the ID of the raw instruction:
						// Seek to the beginning of the instruction actual parameter list.
						seek_to(0, ptr, size_buf, ptr_initial);
						// Clear the instruction's parameter list.
						if (ins.is_rawins) {
							if (ins.rawins_params) {
								delete ins.rawins_params;
								ins.rawins_params = nullptr;
							}
						} else {
							if (ins.params) {
								delete ins.params;
								ins.params = nullptr;
							}
						}
						ins.is_rawins = false;
						try {
							this->DecodeRawEclParams(
								ptr,
								size_buf,
								ptr_initial,
								raw_ecl_ins_hdr.param_count,
								raw_ecl_ins_hdr.param_mask,
								it_map_ins->second,
								ins
							);
						} catch (DecoderException&) {
							/// If a decoder exception occurred when checking the parameter list, fail immediately (for this instruction slot).
							continue;
						}
						return;
					}
					// If no instruction slots match the raw instruction:
					throw(ErrDecoderInvalidParameters(0));
				}(ptr, raw_ecl_ins_hdr.size - sizeof(raw_ecl_ins_hdr));
			} catch (ErrDecoderUnexpEof& err) {
				throw(ErrDecoderInvalidParameters(err.GetOffset()));
			}
		} catch (DecoderException& err) {
			err.AdjustOffset(ptr_initial, ptr);
			throw;
		}
		seek_delta(raw_ecl_ins_hdr.size - sizeof(raw_ecl_ins_hdr), ptr, size_buf, ptr_initial);
		list_data_entry.emplace_back(new DecodedIns(ins));
		list<shared_ptr<DecodedSubDataEntry>>::const_iterator it_list_data_entry = list_data_entry.cend();
		--it_list_data_entry;
		map_it_list_data_entry_offs[offs_ins] = it_list_data_entry;
		offs_ins += raw_ecl_ins_hdr.size;
	}
	uint32_t id_target_next = 0;
	map<size_t, DecodedJmpTarget*> map_jmp_target;
	for (const pair<DecodedParam_Jmp*, size_t>& val_param_jmp_pending : vec_param_jmp_pending) {
		uint32_t id_target;
		try {
			// If a target is already present at the offset, use it directly.
			id_target = map_jmp_target.at(val_param_jmp_pending.second)->id_target;
		} catch (out_of_range&) {
			// Otherwise, create a new jump target at the offset.
			id_target = id_target_next++;
			shared_ptr<DecodedJmpTarget> target(new DecodedJmpTarget(id_target));
			list<shared_ptr<DecodedSubDataEntry>>::const_iterator it_list_data_entry;
			try {
				it_list_data_entry = map_it_list_data_entry_offs.at(val_param_jmp_pending.second);
			} catch (out_of_range&) {
				// TODO: Implement better handling for invalid goto instructions.
				throw(ErrDecoderInvalidSub(sizeof(raw_ecl_sub_hdr)));
			}
			map_it_list_data_entry_offs.at(val_param_jmp_pending.second) =
				list_data_entry.insert(it_list_data_entry, static_pointer_cast<DecodedSubDataEntry, DecodedJmpTarget>(target));
			map_jmp_target[val_param_jmp_pending.second] = target.get();
		}
		val_param_jmp_pending.first->id_target = id_target;
	}
	vec_param_jmp_pending.clear();
	sub->data_entries = vector<shared_ptr<DecodedSubDataEntry>>(list_data_entry.cbegin(), list_data_entry.cend());
}

void RawEclDecoder::DecodeRawEclParams(
	const char*& ptr,
	size_t& size_buf,
	const char* const ptr_initial,
	uint8_t param_count,
	uint16_t param_mask,
	const ins_def_t& ins_def,
	DecodedIns& ins
) const {
	if (ins_def.params.size() == 1 && ins_def.params.at(0) == ReadIns::NumType::Anything) {
		// If the parameter type specified for this instruction slot is "anything", succeed immediately and decode this instruction to a rawins.
		ins.is_rawins = true;
		unique_ptr<DecodedIns::rawins_params_t> params(new DecodedIns::rawins_params_t());
		params->param_count = param_count;
		params->param_mask = param_mask;
		params->str_raw_params = string(ptr, size_buf);
		ins.rawins_params = params.release();
	} else {
		ins.is_rawins = false;
		unique_ptr<vector<shared_ptr<DecodedParam>>> params(new vector<shared_ptr<DecodedParam>>());
		vector<ReadIns::NumType>::const_iterator it_numtype;
		uint8_t i_param;
		for (
			it_numtype = ins_def.params.cbegin(), i_param = 0;
			it_numtype != ins_def.params.cend() && i_param < param_count;
			++it_numtype
			) {
			switch (*it_numtype) {
			case ReadIns::NumType::Int: {
				int32_t rawval;
				extract_data(&rawval, sizeof(int32_t), ptr, size_buf, ptr_initial);
				if (param_mask & (1 << i_param)) {
					int32_t ref_id = rawval;
					if (ref_id > 0) {
						if (ref_id & 0x3) {
							params->emplace_back(new DecodedParam_AbnormalVariable(ref_id, false));
						} else {
							params->emplace_back(new DecodedParam_Variable((uint32_t)ref_id >> 2, false));
						}
					} else if (ref_id < 0 && ref_id >= -100) {
						params->emplace_back(new DecodedParam_Stack(ref_id, false));
					} else if (ref_id < -100) {
						params->emplace_back(new DecodedParam_Env(-ref_id, false));
					} else {
						throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(int32_t)));
					}
				} else {
					params->emplace_back(new DecodedParam_Int(rawval));
				}
				++i_param;
				break;
			}
			case ReadIns::NumType::Float:
			{
				float rawval;
				extract_data(&rawval, sizeof(float), ptr, size_buf, ptr_initial);
				if (param_mask & (1 << i_param)) {
					if (rawval < INT32_MIN || rawval > INT32_MAX) throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(float)));
					int32_t ref_id = (int32_t)rawval;
					if (ref_id > 0) {
						if (ref_id & 0x3) {
							params->emplace_back(new DecodedParam_AbnormalVariable(ref_id, true));
						} else {
							params->emplace_back(new DecodedParam_Variable((uint32_t)ref_id >> 2, true));
						}
					} else if (ref_id < 0 && ref_id >= -100) {
						params->emplace_back(new DecodedParam_Stack(ref_id, true));
					} else if (ref_id < -100) {
						params->emplace_back(new DecodedParam_Env(-ref_id, true));
					} else {
						throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(float)));
					}
				} else {
					params->emplace_back(new DecodedParam_Float(rawval));
				}
				++i_param;
				break;
			}
			case ReadIns::NumType::String:
			{
				if (param_mask & (1 << i_param)) throw(ErrDecoderInvalidParameters(ptr - ptr_initial));
				size_t len_str = strnlen(ptr, size_buf);
				if (len_str < 0) throw(ErrDecoderInvalidParameters(ptr - ptr_initial));
				if (size_buf < len_str + 1) throw(ErrDecoderUnexpEof(ptr, size_buf, ptr_initial));
				params->emplace_back(new DecodedParam_String(string(ptr, len_str)));
				seek_delta(len_str + 1, ptr, size_buf, ptr_initial);
				seek_align4(ptr, size_buf, ptr_initial);
				++i_param;
				break;
			}
			case ReadIns::NumType::Call:
			{
				while (i_param < param_count) {
					uint32_t typeval;
					extract_data(&typeval, sizeof(uint32_t), ptr, size_buf, ptr_initial);
					bool is_from_float;
					bool is_to_float;
					if ((typeval & 0xFF) == 0x66) {
						is_from_float = true;
					} else if ((typeval & 0xFF) == 0x69) {
						is_from_float = false;
					} else {
						throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(uint32_t)));
					}
					if ((typeval & 0xFF00) == 0x6600) {
						is_to_float = true;
					} else if ((typeval & 0xFF00) == 0x6900) {
						is_to_float = false;
					} else {
						throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(uint32_t)));
					}
					union {
						int32_t rawval_int;
						float rawval_float;
					};
					shared_ptr<DecodedParam> param_in;
					if (is_from_float) {
						extract_data(&rawval_float, sizeof(float), ptr, size_buf, ptr_initial);
					} else {
						extract_data(&rawval_int, sizeof(int32_t), ptr, size_buf, ptr_initial);
					}
					if (param_mask & (1 << i_param)) {
						int32_t ref_id;
						if (is_from_float) {
							if (rawval_float < INT32_MIN || rawval_float > INT32_MAX) throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(float)));
							ref_id = (int32_t)rawval_float;
						} else {
							ref_id = rawval_int;
						}
						if (ref_id > 0) {
							if (ref_id & 0x3) {
								param_in = shared_ptr<DecodedParam>(new DecodedParam_AbnormalVariable(ref_id, is_from_float));
							} else {
								param_in = shared_ptr<DecodedParam>(new DecodedParam_Variable((uint32_t)ref_id >> 2, is_from_float));
							}
						} else if (ref_id < 0 && ref_id >= -100) {
							param_in = shared_ptr<DecodedParam>(new DecodedParam_Stack(ref_id, is_from_float));
						} else if (ref_id < -100) {
							param_in = shared_ptr<DecodedParam>(new DecodedParam_Env(-ref_id, is_from_float));
						} else {
							throw(ErrDecoderInvalidParameters(ptr - ptr_initial - sizeof(int32_t)));
						}
					} else {
						if (is_from_float) {
							param_in = shared_ptr<DecodedParam>(new DecodedParam_Float(rawval_float));
						} else {
							param_in = shared_ptr<DecodedParam>(new DecodedParam_Int(rawval_int));
						}
					}
					params->emplace_back(new DecodedParam_Call(param_in, is_from_float, is_to_float));
					++i_param;
				}
				break;
			}
			default:
				throw(ErrDesignApp("RawEclDecoder::DecodeRawEclSub : unknown NumType"));
			}
		}
		// If the parameters don't match, fail immediately (for this instruction slot).
		if (it_numtype != ins_def.params.cend() || i_param != param_count) {
			throw(ErrDecoderInvalidParameters(ptr - ptr_initial));
		}
		ins.params = params.release();
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
