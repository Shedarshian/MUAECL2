#pragma once
#include <cstdint>
#include <exception>
#include <istream>
#include <vector>
#include <memory>
#include "Misc.h"

using namespace std;

/// <summary>Decoded ECL instruction parameter.</summary>
struct DecodedParam abstract {
	enum ParamType {
		Int,
		Float,
		String,
		Variable,
		AbnormalVariable,
		Stack,
		Env,
		Jmp,
		Call
	};
	const ParamType param_type;
	explicit DecodedParam(ParamType param_type);
	virtual ~DecodedParam();
};

struct DecodedParam_Int : public DecodedParam {
	int32_t val;
	explicit DecodedParam_Int(int32_t val);
	virtual ~DecodedParam_Int();
};

struct DecodedParam_Float : public DecodedParam {
	float val;
	explicit DecodedParam_Float(float val);
	virtual ~DecodedParam_Float();
};

struct DecodedParam_String : public DecodedParam {
	string val;
	explicit DecodedParam_String(string val);
	virtual ~DecodedParam_String();
};

struct DecodedParam_Variable : public DecodedParam {
	uint32_t id_var;
	bool is_float;
	explicit DecodedParam_Variable(uint32_t id_var, bool is_float);
	virtual ~DecodedParam_Variable();
};

struct DecodedParam_AbnormalVariable : public DecodedParam {
	int32_t ref_id;
	bool is_float;
	explicit DecodedParam_AbnormalVariable(int32_t ref_id, bool is_float);
	virtual ~DecodedParam_AbnormalVariable();
};

struct DecodedParam_Stack : public DecodedParam {
	int32_t ref_id;
	bool is_float;
	explicit DecodedParam_Stack(int32_t ref_id, bool is_float);
	virtual ~DecodedParam_Stack();
};

struct DecodedParam_Env : public DecodedParam {
	uint32_t env_id;
	bool is_float;
	explicit DecodedParam_Env(uint32_t env_id, bool is_float);
	virtual ~DecodedParam_Env();
};

struct DecodedParam_Jmp : public DecodedParam {
	uint32_t id_target;
	explicit DecodedParam_Jmp(uint32_t id_target);
	virtual ~DecodedParam_Jmp();
};

struct DecodedParam_Call : public DecodedParam {
	shared_ptr<DecodedParam> param;
	bool is_from_float;
	bool is_to_float;
	explicit DecodedParam_Call(shared_ptr<DecodedParam>& param, bool is_from_float, bool is_to_float);
	virtual ~DecodedParam_Call();
};

struct DecodedSubDataEntry abstract {};

/// <summary>Jump target.</summary>
struct DecodedJmpTarget final : public DecodedSubDataEntry {
	uint32_t id_target;
	explicit DecodedJmpTarget(uint32_t id_target);
	~DecodedJmpTarget();
};

/// <summary>Decoded ECL instruction.</summary>
struct DecodedIns final : public DecodedSubDataEntry {
	struct rawins_params_t {
		uint8_t param_count;
		uint16_t param_mask;
		string str_raw_params;
	};
	uint32_t time;
	uint16_t id;
	uint8_t difficulty_mask;
	uint32_t post_pop_count;
	bool is_rawins;
	union {
		uintptr_t _dummy = 0;
		vector<shared_ptr<DecodedParam>>* params;
		rawins_params_t* rawins_params;
	};
	DecodedIns();
	DecodedIns(const DecodedIns& t);
	DecodedIns(DecodedIns&& t);
	virtual ~DecodedIns();
};

/// <summary>Decoded ECL subroutine.</summary>
struct DecodedSub final {
	vector<shared_ptr<DecodedSubDataEntry>> data_entries;
};

/// <summary>Decoded ECL file.</summary>
struct DecodedRoot final {
	vector<string> anim;
	vector<string> ecli;
	vector<shared_ptr<DecodedSub>> subs;
};

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
	void DecodeRawEclSub(const char* ptr, size_t size_buf, shared_ptr<DecodedSub>& sub) const;
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

/// <summary>An exception during the decoding phase, due to invalid or corrupted raw ECL file.</summary>
class DecoderException : public exception {
public:
	/// <summary>Create a <c>DecoderException</c> object.</summary>
	/// <param name="offset">
	/// The offset at which the raw ECL file is invalid or corrupted.
	/// Set this value to be relative to the initial <c>ptr</c> value passed to the calling function.
	/// </param>
	explicit DecoderException(size_t offset) throw();
	virtual ~DecoderException() throw();
	virtual size_t GetOffset() const throw();
	virtual void AdjustOffset(const char* ptr_initial, const char* ptr_initial_inner) throw();
protected:
	/// <summary>
	/// The offset at which the raw ECL file is invalid or corrupted.
	/// The innermost decoding function initially sets this value to be relative to the initial <c>ptr</c> value passed to the function.
	/// The caller of inner decoding functions catches the exception, modifies this value to be relative to the initial <c>ptr</c> value passed to the caller's function, and re-throws the exception.
	/// </summary>
	size_t offset;
};

/// <summary>Decoder unexpected end-of-file exception.</summary>
class ErrDecoderUnexpEof : public DecoderException {
public:
	explicit ErrDecoderUnexpEof(const char* ptr, size_t size_buf, const char* ptr_initial) throw();
	virtual ~ErrDecoderUnexpEof() throw();
	virtual const char* what() const throw();
};

/// <summary>Decoder invalid raw ECL file exception.</summary>
class ErrDecoderInvalidEcl : public DecoderException {
public:
	explicit ErrDecoderInvalidEcl(size_t offset) throw();
	virtual ~ErrDecoderInvalidEcl() throw();
	virtual const char* what() const throw();
};

/// <summary>Decoder invalid includes exception.</summary>
class ErrDecoderInvalidIncludes : public DecoderException {
public:
	explicit ErrDecoderInvalidIncludes(size_t offset) throw();
	virtual ~ErrDecoderInvalidIncludes() throw();
	virtual const char* what() const throw();
};

/// <summary>Decoder invalid raw ECL subroutine exception.</summary>
class ErrDecoderInvalidSub : public DecoderException {
public:
	explicit ErrDecoderInvalidSub(size_t offset) throw();
	virtual ~ErrDecoderInvalidSub() throw();
	virtual const char* what() const throw();
};

/// <summary>Decoder invalid raw ECL parameter(s) exception.</summary>
class ErrDecoderInvalidParameters : public DecoderException {
public:
	explicit ErrDecoderInvalidParameters(size_t offset) throw();
	virtual ~ErrDecoderInvalidParameters() throw();
	virtual const char* what() const throw();
};
