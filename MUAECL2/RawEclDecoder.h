#pragma once
#include "Misc.h"
#include "Misc3.h"

using namespace std;

/// <summary>Raw ECL decoder.</summary>
class RawEclDecoder final {
public:
	RawEclDecoder();
	~RawEclDecoder();
	/// <param name="stream">The input stream.</param>
	shared_ptr<DecodedRoot> DecodeRawEclRoot(istream& stream) const;
protected:
	struct ins_def_t {
		int id;
		string name;
		vector<ReadIns::NumType> params;
		explicit ins_def_t(int id, const string& name, const vector<ReadIns::NumType>& params);
		~ins_def_t();
	};
	multimap<int, ins_def_t> map_ins;
	shared_ptr<DecodedRoot> DecodeRawEclRoot(const char* ptr, size_t size_buf) const;
	void DecodeRawEclIncludes(const char* ptr, size_t size_buf, const shared_ptr<DecodedRoot>& root) const;
	void DecodeRawEclSubDataEntries(const char* ptr, size_t size_buf, DecodedSub& sub) const;
	void DecodeRawEclParams(
		const char*& ptr,
		size_t& size_buf,
		const char* const ptr_initial,
		uint8_t param_count,
		uint16_t param_mask,
		const ins_def_t& ins_def,
		DecodedIns& ins
	) const;
};