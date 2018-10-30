// MUAECL2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Version.h"
#include "Preprocessor.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "RawEclGenerator.h"
#include "RawEclDecoder.h"
#include "Misc4.h"
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <vector>
#include <initializer_list>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <io.h>
#include <fcntl.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace std;

/// <summary>The command line is wrong.</summary>
class WrongCmdlineException final : public exception {
public:
	WrongCmdlineException() : s("Command line syntax wrong.") {}
	WrongCmdlineException(const char* what) : s("Command line syntax wrong: "s + what) {}
	WrongCmdlineException(const string& what) : s("Command line syntax wrong: "s + what) {}
	WrongCmdlineException(string&& what) : s("Command line syntax wrong: "s + what) {}
	virtual const char* what() const throw() { return s.c_str(); }
private:
	const string s;
};

/// <summary>Command line argument definition.</summary>
class cmdarg_def_t final {
public:
	/// <param name="vec_alias">Aliases of this argument.</param>
	/// <param name="is_value_arg">Whether this argument has a value (like "-o"), or is a command line switch.</param>
	/// <param name="is_multivalue_arg">Whether this value argument can be specified multiple times to represent multiple values.</param>
	cmdarg_def_t(const vector<string>& vec_alias, bool is_value_arg = false, bool is_multivalue_arg = false) : vec_alias(vec_alias), is_value_arg(is_value_arg), is_multivalue_arg(is_multivalue_arg) {}
	/// <param name="vec_alias">Aliases of this argument.</param>
	/// <param name="is_value_arg">Whether this argument has a value (like "-o"), or is a command line switch.</param>
	/// <param name="is_multivalue_arg">Whether this value argument can be specified multiple times to represent multiple values.</param>
	cmdarg_def_t(vector<string>&& vec_alias, bool is_value_arg = false, bool is_multivalue_arg = false) : vec_alias(vec_alias), is_value_arg(is_value_arg), is_multivalue_arg(is_multivalue_arg) {}
	/// <param name="il_alias">Aliases of this argument.</param>
	/// <param name="is_value_arg">Whether this argument has a value (like "-o"), or is a command line switch.</param>
	/// <param name="is_multivalue_arg">Whether this value argument can be specified multiple times to represent multiple values.</param>
	cmdarg_def_t(initializer_list<string>&& il_alias, bool is_value_arg = false, bool is_multivalue_arg = false) : vec_alias(il_alias), is_value_arg(is_value_arg), is_multivalue_arg(is_multivalue_arg) {}
	bool isValueArg() const { return this->is_value_arg; }
	bool isMultiValueArg() const { return this->is_multivalue_arg; }
	const vector<string>* getVecAlias() const { return &this->vec_alias; }
private:
	vector<string> vec_alias;
	bool is_value_arg = false;
	bool is_multivalue_arg = false;
};

/// <summary>Command line argument input.</summary>
struct cmdarg_input_t {
	bool is_specified = false;
	string value;
	vector<string> vec_value;
};

/// <summary>Parsed arguments passed to the preprocessing routine.</summary>
struct PreprocessArguments final {
	istream* in = nullptr;
	ostream* out = nullptr;
	rapidjson::Document* jsondoc_dbginfo = nullptr;
	rapidjson::Value* jsonval_dbginfo_srcfile_in = nullptr;
	rapidjson::Value* jsonval_dbginfo_srcfile_out = nullptr;
	uint64_t* id_dbginfo_srcpos_next = nullptr;
	vector<filesystem::path> searchpath;
	filesystem::path currentpath;
	vector<string> ecli;
	vector<string> anim;
};

/// <summary>Parsed arguments passed to the compiling routine.</summary>
struct CompileArguments final {
	istream* in = nullptr;
	ostream* out = nullptr;
	rapidjson::Document* jsondoc_dbginfo = nullptr;
	rapidjson::Value* jsonval_dbginfo_srcfile = nullptr;
	rapidjson::Value* jsonval_dbginfo_eclfile = nullptr;
	uint64_t* id_dbginfo_srcpos_next = nullptr;
	string filename;
	vector<string> ecli;
	vector<string> anim;
};

/// <summary>The type field in the debug information JSON.</summary>
static const std::string str_dbginfo_type(u8"MUAECL2"s);

/// <summary>Command line argument definition map.
/// From the internally used identifier of an argument to the <c>cmdarg_def_t</c> object that contains its definition.
/// </summary>
static const unordered_map<string, cmdarg_def_t> map_cmdarg_def({
	{ "help"s, cmdarg_def_t({ "-h"s, "--help"s }) },
	{ "version"s, cmdarg_def_t({ "-V"s, "--version"s }) },
	{ "compile"s, cmdarg_def_t({ "-c"s, "--compile"s }) },
	{ "preprocess"s, cmdarg_def_t({ "-p"s, "--preprocess"s }) },
	{ "input-file"s, cmdarg_def_t({ "-i"s, "--input-file"s }, true) },
	{ "output-file"s, cmdarg_def_t({ "-o"s, "--output-file"s }, true) },
	{ "search-path"s, cmdarg_def_t({ "-s"s, "--search-path"s }, true, true) },
	{ "no-preprocess"s, cmdarg_def_t({ "-n"s, "--no-preprocess"s }) }
	});

/// <summary>Display help.</summary>
static void display_help() throw();
/// <summary>Display version.</summary>
static void display_version() throw();
/// <summary>Command line action: Compile an MUAECL2 source file to a raw ECL file.</summary>
static void cmd_compile(unordered_map<string, cmdarg_input_t>& map_cmdarg_input);
/// <summary>Command line action: Preprocess (and not compile) an MUAECL2 source file to a preprocessed source file.</summary>
static void cmd_preprocess(unordered_map<string, cmdarg_input_t>& map_cmdarg_input);
static void preprocess(PreprocessArguments& preprocess_args);
static void compile(CompileArguments& compile_args);
/// <summary>Process statement marks in the debug information document.</summary>
static void dbginfo_process_stmt_marks(rapidjson::Document& jsondoc_dbginfo, rapidjson::Value& jsonval_dbginfo_srcfile, rapidjson::Value& jsonval_dbginfo_eclfile);
/// <summary>Strip intermediate information in the debug information document.</summary>
static void dbginfo_strip_intermediate(rapidjson::Document& jsondoc_dbginfo);

int main(int argc, char* argv[]) {
	try {
		unordered_map<string, string> map_cmdarg_alias_to_id;
		for (const pair<string, cmdarg_def_t>& val_cmdarg_def : map_cmdarg_def) {
			for (const string& val_alias : *val_cmdarg_def.second.getVecAlias()) {
				map_cmdarg_alias_to_id[val_alias] = val_cmdarg_def.first;
			}
		}

		vector<string> vec_rawcmdarg;
		for (int i = 1; i < argc; ++i) {
			vec_rawcmdarg.emplace_back(argv[i]);
		}

		unordered_map<string, cmdarg_input_t> map_cmdarg_input;

		map_cmdarg_input.reserve(vec_rawcmdarg.size());
		for (
			vector<string>::const_iterator it_vec_rawcmdarg = vec_rawcmdarg.begin();
			it_vec_rawcmdarg != vec_rawcmdarg.end();
			++it_vec_rawcmdarg
			) {
			string str_alias = *it_vec_rawcmdarg;
			string str_id;
			try {
				str_id = map_cmdarg_alias_to_id.at(str_alias);
			} catch (out_of_range&) {
				throw(WrongCmdlineException("unknown command line argument alias "s + str_alias));
			}
			const cmdarg_def_t* cmdarg_def = &map_cmdarg_def.at(str_id);
			if (map_cmdarg_input.count(str_id)) cerr << "Warning: command line argument "s + str_id + " specified more than once." << endl;
			if (cmdarg_def->isValueArg()) {
				if (cmdarg_def->isMultiValueArg()) {
					++it_vec_rawcmdarg;
					if (it_vec_rawcmdarg == vec_rawcmdarg.end()) throw(WrongCmdlineException("unexpected end of command line, expected value for argument "s + str_id));
					map_cmdarg_input[str_id].is_specified = true;
					map_cmdarg_input[str_id].vec_value.push_back(*it_vec_rawcmdarg);
				} else {
					++it_vec_rawcmdarg;
					if (it_vec_rawcmdarg == vec_rawcmdarg.end()) throw(WrongCmdlineException("unexpected end of command line, expected value for argument "s + str_id));
					map_cmdarg_input[str_id].is_specified = true;
					map_cmdarg_input[str_id].value = *it_vec_rawcmdarg;
				}
			} else {
				map_cmdarg_input[str_id].is_specified = true;
			}
		}

		if (map_cmdarg_input["help"s].is_specified) {
			display_help();
		} else if (map_cmdarg_input["version"s].is_specified) {
			display_version();
		} else if (map_cmdarg_input["compile"s].is_specified) {
			cmd_compile(map_cmdarg_input);
		} else if (map_cmdarg_input["preprocess"s].is_specified) {
			cmd_preprocess(map_cmdarg_input);
		} else {
			throw(WrongCmdlineException("no action specified"));
		}
	} catch (WrongCmdlineException &e) {
		cerr << e.what() << endl;
		display_help();
	} catch (ExceptionWithLineNo &e) {
		cerr << e.lineNo << " " << e.what() << endl;
	} catch (DecoderException &e) {
		char str_offs[1024];
		sprintf_s(str_offs, 1024, "0x%08zX", e.GetOffset());
		cerr << "Decoder : 0x" << str_offs << " : " << e.what() << endl;
	} catch (ErrDesignApp &e) {
		cerr << e.what() << endl;
	} catch (ErrFileNotFound& e) {
		cerr << e.what() << endl;
	} catch (exception &e) {
		cerr << e.what() << endl;
	}
#ifdef _DEBUG
	system("pause");
#endif
	return 0;
}

static void display_help() throw() {
	static const string str_help = "\
Command line syntax help:\n\
\n\
  MUAECL2 {-h|--help}\n\
Display this help.\n\
\n\
  MUAECL2 {-V|--version}\n\
Display version information.\n\
\n\
  MUAECL2 {-c|--compile} [options]\n\
Compile an MUAECL2 source file to a raw ECL file.\n\
\n\
    {-i|--input-file} <input-filename>              The filename of the input file.\n\
    {-o|--output-file} <output-filename>            The filename of the output file.\n\
    {-s|--search-path} <search-path>                The path in which includes are searched.\n\
                                                    Multiple paths can be specified.\n\
    {-n|--no-preprocess}                            Bypass the preprocessing step.\n\
\n\
  MUAECL2 {-p|--preprocess} [options]\n\
Preprocess (and not compile) an MUAECL2 source file to a preprocessed source file.\n\
\n\
    {-i|--input-file} <input-filename>              The filename of the input file.\n\
    {-o|--output-file} <output-filename>            The filename of the output file.\n\
    {-s|--search-path} <search-path>                The path in which includes are searched.\n\
                                                    Multiple paths can be specified.\n\
\n\
Examples:\n\
  MUAECL2 -h\n\
  MUAECL2 -c -i st01.mecl -o st01.ecl\n\
  MUAECL2 -p -i st01.mecl -o st01p.mecl\n"s;
	cerr << str_help << endl;
}

static void display_version() throw() {
	static const string str_version = "MUAECL "s + MUAECL_VERSION_STRING;
	cerr << str_version << endl;
}

static void cmd_compile(unordered_map<string, cmdarg_input_t>& map_cmdarg_input) {
	ReadIns::Read();

	istream* in = nullptr;
	ostream* out = nullptr;
	rapidjson::Document jsondoc_dbginfo;
	uint64_t id_dbginfo_srcpos_next = 0;
	string filename;	//used for macro substitute
	filesystem::path currentpath;
	vector<filesystem::path> searchpath;	//used for preprocess

	unique_ptr<ifstream> in_f;
	if (map_cmdarg_input["input-file"s].is_specified) {
		filename = map_cmdarg_input["input-file"s].value;
		in_f = unique_ptr<ifstream>(new ifstream(filename, ios_base::binary));
		in = in_f.get();
		currentpath = filesystem::path(filename).remove_filename();
	} else {
		_setmode(_fileno(stdin), _O_BINARY);
		in = &cin;
		filename = "std::cin"s;
	}

	unique_ptr<ofstream> out_f;
	if (map_cmdarg_input["output-file"s].is_specified) {
		out_f = unique_ptr<ofstream>(new ofstream(map_cmdarg_input["output-file"s].value, ios_base::binary));
		out = out_f.get();
	} else {
		_setmode(_fileno(stdout), _O_BINARY);
		out = &cout;
	}

	unique_ptr<ofstream> dbginfo_f;
	if (map_cmdarg_input["output-file"s].is_specified) {
		filesystem::path fspath_dbginfo(map_cmdarg_input["output-file"s].value);
		fspath_dbginfo.replace_extension(filesystem::path("mecl-dbginfo"));
		dbginfo_f = unique_ptr<ofstream>(new ofstream(fspath_dbginfo, ios_base::binary));
	}
	jsondoc_dbginfo.SetObject();
	jsondoc_dbginfo.AddMember(u8"type", rapidjson::Value(str_dbginfo_type.c_str(), str_dbginfo_type.size(), jsondoc_dbginfo.GetAllocator()), jsondoc_dbginfo.GetAllocator());
	jsondoc_dbginfo.AddMember(u8"srcfiles", rapidjson::Value(rapidjson::Type::kArrayType), jsondoc_dbginfo.GetAllocator());
	jsondoc_dbginfo.AddMember(u8"eclfile", rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());

	for (const string& val_search_path : map_cmdarg_input["search-path"s].vec_value)
		searchpath.emplace_back(val_search_path);

	if (map_cmdarg_input["no-preprocess"s].is_specified) {
		jsondoc_dbginfo[u8"srcfiles"].PushBack(rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcfile_compile = *(jsondoc_dbginfo[u8"srcfiles"].End() - 1);
		rapidjson::Value& jsonval_dbginfo_eclfile = jsondoc_dbginfo[u8"eclfile"];

		CompileArguments compile_args;
		compile_args.in = in;
		compile_args.out = out;
		compile_args.jsondoc_dbginfo = &jsondoc_dbginfo;
		compile_args.jsonval_dbginfo_srcfile = &jsonval_dbginfo_srcfile_compile;
		compile_args.jsonval_dbginfo_eclfile = &jsonval_dbginfo_eclfile;
		compile_args.id_dbginfo_srcpos_next = &id_dbginfo_srcpos_next;
		compile_args.filename = filename;
		compile(compile_args);
	} else {
		stringstream preprocessed;

		jsondoc_dbginfo[u8"srcfiles"].PushBack(rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcfile_preprocess_in = *(jsondoc_dbginfo[u8"srcfiles"].End() - 1);
		jsondoc_dbginfo[u8"srcfiles"].PushBack(rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcfile_preprocess_out = *(jsondoc_dbginfo[u8"srcfiles"].End() - 1);

		PreprocessArguments preprocess_args;
		preprocess_args.in = in;
		preprocess_args.out = &preprocessed;
		preprocess_args.jsondoc_dbginfo = &jsondoc_dbginfo;
		preprocess_args.jsonval_dbginfo_srcfile_in = &jsonval_dbginfo_srcfile_preprocess_in;
		preprocess_args.jsonval_dbginfo_srcfile_out = &jsonval_dbginfo_srcfile_preprocess_out;
		preprocess_args.id_dbginfo_srcpos_next = &id_dbginfo_srcpos_next;
		preprocess_args.currentpath = currentpath;
		preprocess_args.searchpath = searchpath;
		preprocess(preprocess_args);

		jsondoc_dbginfo[u8"srcfiles"].PushBack(rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcfile_compile = *(jsondoc_dbginfo[u8"srcfiles"].End() - 1);
		rapidjson::Value& jsonval_dbginfo_eclfile = jsondoc_dbginfo[u8"eclfile"];

		CompileArguments compile_args;
		compile_args.in = &preprocessed;
		compile_args.out = out;
		compile_args.jsondoc_dbginfo = &jsondoc_dbginfo;
		compile_args.jsonval_dbginfo_srcfile = &jsonval_dbginfo_srcfile_compile;
		compile_args.jsonval_dbginfo_eclfile = &jsonval_dbginfo_eclfile;
		compile_args.id_dbginfo_srcpos_next = &id_dbginfo_srcpos_next;
		compile_args.filename = filename;
		compile_args.ecli = preprocess_args.ecli;
		compile_args.anim = preprocess_args.anim;
		compile(compile_args);
	}

	dbginfo_strip_intermediate(jsondoc_dbginfo);
	if (dbginfo_f) {
		rapidjson::StringBuffer jsonstrbuf_dbginfo;
		rapidjson::Writer<rapidjson::StringBuffer> jsonwriter_dbginfo(jsonstrbuf_dbginfo);
		jsondoc_dbginfo.Accept(jsonwriter_dbginfo);
		dbginfo_f->write(jsonstrbuf_dbginfo.GetString(), jsonstrbuf_dbginfo.GetLength());
	}
}

static void cmd_preprocess(unordered_map<string, cmdarg_input_t>& map_cmdarg_input) {
	ReadIns::Read();

	PreprocessArguments preprocess_args;
	rapidjson::Document jsondoc_dbginfo;
	uint64_t id_dbginfo_srcpos_next = 0;
	filesystem::path currentpath;
	vector<filesystem::path> searchpath;	//used for preprocess

	unique_ptr<ifstream> in_f;
	if (map_cmdarg_input["input-file"s].is_specified) {
		in_f = unique_ptr<ifstream>(new ifstream(map_cmdarg_input["input-file"s].value, ios_base::binary));
		preprocess_args.in = in_f.get();
		currentpath = filesystem::path(map_cmdarg_input["input-file"s].value).remove_filename();
	} else {
		_setmode(_fileno(stdin), _O_BINARY);
		preprocess_args.in = &cin;
	}

	unique_ptr<ofstream> out_f;
	if (map_cmdarg_input["output-file"s].is_specified) {
		out_f = unique_ptr<ofstream>(new ofstream(map_cmdarg_input["output-file"s].value, ios_base::binary));
		preprocess_args.out = out_f.get();
	} else {
		_setmode(_fileno(stdout), _O_BINARY);
		preprocess_args.out = &cout;
	}

	unique_ptr<ofstream> dbginfo_f;
	if (map_cmdarg_input["output-file"s].is_specified) {
		filesystem::path fspath_dbginfo(map_cmdarg_input["output-file"s].value);
		fspath_dbginfo.replace_extension(filesystem::path("mecl-dbginfo"));
		dbginfo_f = unique_ptr<ofstream>(new ofstream(fspath_dbginfo, ios_base::binary));
	}
	jsondoc_dbginfo.SetObject();
	jsondoc_dbginfo.AddMember(u8"type", rapidjson::Value(str_dbginfo_type.c_str(), str_dbginfo_type.size(), jsondoc_dbginfo.GetAllocator()), jsondoc_dbginfo.GetAllocator());
	jsondoc_dbginfo.AddMember(u8"srcfiles", rapidjson::Value(rapidjson::Type::kArrayType), jsondoc_dbginfo.GetAllocator());

	jsondoc_dbginfo[u8"srcfiles"].PushBack(rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());
	rapidjson::Value& jsonval_dbginfo_srcfile_preprocess_in = *(jsondoc_dbginfo[u8"srcfiles"].End() - 1);
	jsondoc_dbginfo[u8"srcfiles"].PushBack(rapidjson::Value(rapidjson::Type::kObjectType), jsondoc_dbginfo.GetAllocator());
	rapidjson::Value& jsonval_dbginfo_srcfile_preprocess_out = *(jsondoc_dbginfo[u8"srcfiles"].End() - 1);

	preprocess_args.jsondoc_dbginfo = &jsondoc_dbginfo;
	preprocess_args.jsonval_dbginfo_srcfile_in = &jsonval_dbginfo_srcfile_preprocess_in;
	preprocess_args.jsonval_dbginfo_srcfile_out = &jsonval_dbginfo_srcfile_preprocess_out;
	preprocess_args.id_dbginfo_srcpos_next = &id_dbginfo_srcpos_next;

	for (const string& val_search_path : map_cmdarg_input["search-path"s].vec_value)
		searchpath.emplace_back(val_search_path);

	preprocess_args.currentpath = currentpath;
	preprocess_args.searchpath = searchpath;
	preprocess(preprocess_args);

	dbginfo_strip_intermediate(jsondoc_dbginfo);
	if (dbginfo_f) {
		rapidjson::StringBuffer jsonstrbuf_dbginfo;
		rapidjson::Writer<rapidjson::StringBuffer> jsonwriter_dbginfo(jsonstrbuf_dbginfo);
		jsondoc_dbginfo.Accept(jsonwriter_dbginfo);
		dbginfo_f->write(jsonstrbuf_dbginfo.GetString(), jsonstrbuf_dbginfo.GetLength());
	}
}

static void preprocess(PreprocessArguments& preprocess_args) {
	if (!preprocess_args.in) throw(ErrDesignApp("!preprocess_args.in"));
	if (!preprocess_args.out) throw(ErrDesignApp("!preprocess_args.out"));
	if (!preprocess_args.jsondoc_dbginfo) throw(ErrDesignApp("!preprocess_args.jsondoc_dbginfo"));
	if (!preprocess_args.jsonval_dbginfo_srcfile_in) throw(ErrDesignApp("!preprocess_args.jsonval_dbginfo_srcfile_in"));
	if (!preprocess_args.jsonval_dbginfo_srcfile_out) throw(ErrDesignApp("!preprocess_args.jsonval_dbginfo_srcfile_out"));
	if (!preprocess_args.id_dbginfo_srcpos_next) throw(ErrDesignApp("!preprocess_args.id_dbginfo_srcpos_next"));

	stringstream sstream_in;
	sstream_in << preprocess_args.in->rdbuf();

	{
		string str_base64_hash_in(base64_encode_string(hash_string(sstream_in.str())));
		preprocess_args.jsonval_dbginfo_srcfile_in->AddMember(u8"hash", rapidjson::Value(str_base64_hash_in.c_str(), str_base64_hash_in.size(), preprocess_args.jsondoc_dbginfo->GetAllocator()), preprocess_args.jsondoc_dbginfo->GetAllocator());
	}

	{
		if (!preprocess_args.jsonval_dbginfo_srcfile_in->HasMember(u8"srcposes")) preprocess_args.jsonval_dbginfo_srcfile_in->AddMember(u8"srcposes", rapidjson::Value(rapidjson::Type::kArrayType), preprocess_args.jsondoc_dbginfo->GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcposes = (*preprocess_args.jsonval_dbginfo_srcfile_in)[u8"srcposes"];
	}

	stringstream sstream_out;
	pair<vector<string>, vector<string>> ecli_and_anim = Preprocessor::process(sstream_in, sstream_out, preprocess_args.currentpath, preprocess_args.searchpath);
	preprocess_args.ecli = ecli_and_anim.first;
	preprocess_args.anim = ecli_and_anim.second;
	// TODO: Write debug information.

	{
		if (!preprocess_args.jsonval_dbginfo_srcfile_out->HasMember(u8"srcposes")) preprocess_args.jsonval_dbginfo_srcfile_out->AddMember(u8"srcposes", rapidjson::Value(rapidjson::Type::kArrayType), preprocess_args.jsondoc_dbginfo->GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcposes = (*preprocess_args.jsonval_dbginfo_srcfile_out)[u8"srcposes"];
	}

	{
		string str_base64_hash_out(base64_encode_string(hash_string(sstream_out.str())));
		preprocess_args.jsonval_dbginfo_srcfile_out->AddMember(u8"hash", rapidjson::Value(str_base64_hash_out.c_str(), str_base64_hash_out.size(), preprocess_args.jsondoc_dbginfo->GetAllocator()), preprocess_args.jsondoc_dbginfo->GetAllocator());
	}

	(*preprocess_args.out) << sstream_out.rdbuf();
}

static void compile(CompileArguments& compile_args) {
	if (!compile_args.in) throw(ErrDesignApp("!compile_args.in"));
	if (!compile_args.out) throw(ErrDesignApp("!compile_args.out"));
	if (!compile_args.jsondoc_dbginfo) throw(ErrDesignApp("!compile_args.jsondoc_dbginfo"));
	if (!compile_args.jsonval_dbginfo_srcfile) throw(ErrDesignApp("!compile_args.jsonval_dbginfo_srcfile"));
	if (!compile_args.jsonval_dbginfo_eclfile) throw(ErrDesignApp("!compile_args.jsonval_dbginfo_eclfile"));
	if (!compile_args.id_dbginfo_srcpos_next) throw(ErrDesignApp("!compile_args.id_dbginfo_srcpos_next"));

	stringstream sstream_in;
	sstream_in << compile_args.in->rdbuf();

	{
		string str_base64_hash_in(base64_encode_string(hash_string(sstream_in.str())));
		compile_args.jsonval_dbginfo_srcfile->AddMember(u8"hash", rapidjson::Value(str_base64_hash_in.c_str(), str_base64_hash_in.size(), compile_args.jsondoc_dbginfo->GetAllocator()), compile_args.jsondoc_dbginfo->GetAllocator());
	}

	Parser::initialize();
	Tokenizer tokenizer(sstream_in, compile_args.filename);
	Parser parser(tokenizer);
	tRoot* tree = parser.analyse();
	parser.TypeCheck();
	RawEclGenerator raw_ecl_generator(parser.Output(compile_args.ecli, compile_args.anim, *compile_args.jsondoc_dbginfo, *compile_args.jsonval_dbginfo_eclfile));
	string str_out;
	raw_ecl_generator.generate(str_out, *compile_args.jsondoc_dbginfo, *compile_args.jsonval_dbginfo_eclfile);
	Parser::clear();

	{
		if (!compile_args.jsonval_dbginfo_srcfile->HasMember(u8"srcposes")) compile_args.jsonval_dbginfo_srcfile->AddMember(u8"srcposes", rapidjson::Value(rapidjson::Type::kArrayType), compile_args.jsondoc_dbginfo->GetAllocator());
		rapidjson::Value& jsonval_dbginfo_srcposes = (*compile_args.jsonval_dbginfo_srcfile)[u8"srcposes"];
		map<int, streamoff> map_lineno_to_pos;
		{
			map<int, streampos> map_lineno_to_streampos(tokenizer.popLineNoToPos());
			for (const pair<int, streampos>& val_lineno_to_streampos : map_lineno_to_streampos)
				map_lineno_to_pos.emplace(val_lineno_to_streampos);
		}
		jsonval_dbginfo_srcposes.Reserve(map_lineno_to_pos.size(), compile_args.jsondoc_dbginfo->GetAllocator());
		unordered_map<streamoff, rapidjson::Value&> map_jsonval_dbginfo_srcpos;
		for (rapidjson::Value& jsonval_dbginfo_srcpos : jsonval_dbginfo_srcposes.GetArray()) {
			map_jsonval_dbginfo_srcpos.emplace(jsonval_dbginfo_srcpos[u8"pos"].GetInt64(), jsonval_dbginfo_srcpos);
		}
		for (const pair<int, streamoff>& val_lineno_to_pos : map_lineno_to_pos) {
			if (!map_jsonval_dbginfo_srcpos.count(val_lineno_to_pos.second)) {
				jsonval_dbginfo_srcposes.PushBack(rapidjson::Value(rapidjson::Type::kObjectType), compile_args.jsondoc_dbginfo->GetAllocator());
				rapidjson::Value& jsonval_dbginfo_srcpos = *(jsonval_dbginfo_srcposes.End() - 1);
				map_jsonval_dbginfo_srcpos.emplace(val_lineno_to_pos.second, jsonval_dbginfo_srcpos);
				jsonval_dbginfo_srcpos.AddMember(u8"pos", rapidjson::Value((int64_t)val_lineno_to_pos.second), compile_args.jsondoc_dbginfo->GetAllocator());
				jsonval_dbginfo_srcpos.AddMember(u8"id", rapidjson::Value((*compile_args.id_dbginfo_srcpos_next)++), compile_args.jsondoc_dbginfo->GetAllocator());
			}
			rapidjson::Value& jsonval_dbginfo_srcpos = map_jsonval_dbginfo_srcpos.at(val_lineno_to_pos.second);
			jsonval_dbginfo_srcpos.AddMember(u8"lineno", rapidjson::Value((int64_t)val_lineno_to_pos.first), compile_args.jsondoc_dbginfo->GetAllocator());
		}
	}

	dbginfo_process_stmt_marks(*compile_args.jsondoc_dbginfo, *compile_args.jsonval_dbginfo_srcfile, *compile_args.jsonval_dbginfo_eclfile);

	{
		string str_base64_hash(base64_encode_string(hash_string(str_out)));
		compile_args.jsonval_dbginfo_eclfile->AddMember(u8"hash", rapidjson::Value(str_base64_hash.c_str(), str_base64_hash.size(), compile_args.jsondoc_dbginfo->GetAllocator()), compile_args.jsondoc_dbginfo->GetAllocator());
	}

	compile_args.out->write(str_out.c_str(), str_out.size());
}

static void dbginfo_process_stmt_marks(rapidjson::Document& jsondoc_dbginfo, rapidjson::Value& jsonval_dbginfo_srcfile, rapidjson::Value& jsonval_dbginfo_eclfile) {
	map<int, uint64_t> map_lineno_to_id_srcpos;
	for (rapidjson::Value& jsonval_dbginfo_srcpos : jsonval_dbginfo_srcfile[u8"srcposes"].GetArray()) {
		if (jsonval_dbginfo_srcpos.HasMember(u8"lineno"))
			map_lineno_to_id_srcpos.emplace((int)(jsonval_dbginfo_srcpos[u8"lineno"].GetInt64() & ~(unsigned int)0), jsonval_dbginfo_srcpos[u8"id"].GetUint64());
	}
	for (rapidjson::Value& jsonval_dbginfo_eclsub : jsonval_dbginfo_eclfile[u8"eclsubs"].GetArray()) {
		for (rapidjson::Value& jsonval_dbginfo_stmt_mark : jsonval_dbginfo_eclsub[u8"stmt_marks"].GetArray()) {
			if (jsonval_dbginfo_stmt_mark.HasMember(u8"lineno")) {
				int lineno = jsonval_dbginfo_stmt_mark[u8"lineno"].GetInt() & ~(unsigned int)0;
				jsonval_dbginfo_stmt_mark.AddMember(u8"id_srcpos", rapidjson::Value(map_lineno_to_id_srcpos.at(lineno)), jsondoc_dbginfo.GetAllocator());
			}
		}
	}
}

static void dbginfo_strip_intermediate(rapidjson::Document& jsondoc_dbginfo) {
	if (jsondoc_dbginfo.HasMember(u8"srcfiles")) for (rapidjson::Value& jsonval_dbginfo_srcfile : jsondoc_dbginfo[u8"srcfiles"].GetArray()) {
		if (jsonval_dbginfo_srcfile.HasMember(u8"srcposes")) for (rapidjson::Value& jsonval_dbginfo_srcpos : jsonval_dbginfo_srcfile[u8"srcposes"].GetArray()) {
			if (jsonval_dbginfo_srcpos.HasMember(u8"lineno")) jsonval_dbginfo_srcpos.RemoveMember(u8"lineno");
		}
	}
	if (jsondoc_dbginfo.HasMember(u8"eclfiles")) for (rapidjson::Value& jsonval_dbginfo_eclfile : jsondoc_dbginfo[u8"eclfiles"].GetArray()) {
		if (jsonval_dbginfo_eclfile.HasMember(u8"eclsubs")) for (rapidjson::Value& jsonval_dbginfo_eclsub : jsonval_dbginfo_eclfile[u8"eclsubs"].GetArray()) {
			if (jsonval_dbginfo_eclsub.HasMember(u8"stmt_marks")) for (rapidjson::Value& jsonval_dbginfo_stmt_mark : jsonval_dbginfo_eclsub[u8"stmt_marks"].GetArray()) {
				if (jsonval_dbginfo_stmt_mark.HasMember(u8"lineno")) jsonval_dbginfo_stmt_mark.RemoveMember(u8"lineno");
			}
		}
	}
}
