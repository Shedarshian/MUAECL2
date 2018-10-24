#pragma once
#include <string>
#include <ostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <rapidjson/document.h>
#include "Misc.h"
#include "Misc2.h"

using namespace std;

/// <summary>Raw ECL file generator.</summary>
class RawEclGenerator final {
public:
	/// <summary>Create a RawEclGenerator object.</summary>
	/// <param name="root">The <c>fRoot</c> structure from which to generate the raw ECL file.</param>
	RawEclGenerator(const fRoot& root);
	~RawEclGenerator();
	/// <summary>Generate the raw ECL file and output it to an output stream.</summary>
	/// <param name="str">A string to which the output is written.</param>
	/// <param name="jsondoc_dbginfo">The debug information JSON document.</param>
	void generate(string& str, rapidjson::Document& jsondoc_dbginfo, rapidjson::Value& jsonval_dbginfo_eclfile) const;
private:
	const fRoot root;
	size_t generate(char* ptr, size_t size_buf, rapidjson::Document* jsondoc_dbginfo, rapidjson::Value* jsonval_dbginfo_eclfile) const;
	size_t make_raw_includes(char* ptr, size_t size_buf, rapidjson::Document* jsondoc_dbginfo, rapidjson::Value* jsonval_dbginfo_eclfile) const;
	size_t make_raw_sub(char* ptr, size_t size_buf, const fSub& sub, rapidjson::Document* jsondoc_dbginfo, rapidjson::Value* jsonval_dbginfo_sub) const;
};

struct SubSerializationContext final {
	SubSerializationContext(rapidjson::Document* jsondoc_dbginfo, rapidjson::Value* jsonval_dbginfo_sub, uint32_t count_var, const vector<shared_ptr<fSubDataEntry>>& data_entries);
	~SubSerializationContext();
	rapidjson::Document* jsondoc_dbginfo = nullptr;
	rapidjson::Value* jsonval_dbginfo_sub = nullptr;
	uint32_t count_var;
	vector<shared_ptr<fSubDataEntry>> vec_data_entry;
	vector<size_t> vec_offs_data_entry;
	size_t i_data_entry_current;
	unordered_map<uint32_t, size_t> map_offs_target;
	map<size_t, int> map_lineno_stmt_mark;
};
