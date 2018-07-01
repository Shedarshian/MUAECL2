#pragma once
#include <ostream>
#include <unordered_map>
#include "Misc.h"
#include "Misc2.h"

using namespace std;

/// <summary>Raw ECL file generator.</summary>
class RawEclGenerator final {
public:
	/// <summary>Create an RawEclGenerator object.</summary>
	/// <param name="root">The <c>fRoot</c> structure from which to generate the raw ECL file.</param>
	RawEclGenerator(const fRoot& root);
	~RawEclGenerator();
	/// <summary>Generate the raw ECL file and output it to an output stream.</summary>
	/// <param name="stream">The output stream.</param>
	void generate(ostream& stream) const;
private:
	const fRoot root;
	size_t generate(char* ptr, size_t size_buf) const;
	size_t make_raw_includes(char* ptr, size_t size_buf) const;
	size_t make_raw_sub(char* ptr, size_t size_buf, const fSub& sub) const;
};

struct SubSerializationContext final {
	SubSerializationContext(const vector<string>& variables, const vector<Ins>& inses);
	~SubSerializationContext();
	vector<string> vec_var;
	unordered_map<string, int32_t> map_var;
	vector<Ins> vec_ins;
	vector<size_t> vec_offs_ins;
	size_t i_ins_current;
};