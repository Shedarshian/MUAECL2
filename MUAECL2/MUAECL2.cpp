// MUAECL2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "RawEclGenerator.h"
#include "RawEclDecoder.h"
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <initializer_list>
#include <unordered_map>

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
	cmdarg_def_t(const vector<string>& vec_alias, bool is_value_arg) : vec_alias(vec_alias), is_value_arg(is_value_arg) {}
	/// <param name="vec_alias">Aliases of this argument.</param>
	/// <param name="is_value_arg">Whether this argument has a value (like "-o"), or is a command line switch.</param>
	cmdarg_def_t(vector<string>&& vec_alias, bool is_value_arg) : vec_alias(vec_alias), is_value_arg(is_value_arg) {}
	/// <param name="il_alias">Aliases of this argument.</param>
	/// <param name="is_value_arg">Whether this argument has a value (like "-o"), or is a command line switch.</param>
	cmdarg_def_t(initializer_list<string>&& il_alias, bool is_value_arg) : vec_alias(il_alias), is_value_arg(is_value_arg) {}
	bool isValueArg() const { return this->is_value_arg; }
	const vector<string>* getVecAlias() const { return &this->vec_alias; }
private:
	vector<string> vec_alias;
	bool is_value_arg = false;
};

/// <summary>Command line argument input.</summary>
struct cmdarg_input_t {
	bool is_specified = false;
	string value;
};

/// <summary>Parsed arguments passed to the preprocessing routine.</summary>
struct PreprocessArguments final {
	istream* in = nullptr;
	ostream* out = nullptr;
};

/// <summary>Parsed arguments passed to the compiling routine.</summary>
struct CompileArguments final {
	istream* in = nullptr;
	ostream* out = nullptr;
};

/// <summary>Command line argument definition map.
/// From the internally used identifier of an argument to the <c>cmdarg_def_t</c> object that contains its definition.
/// </summary>
static const unordered_map<string, cmdarg_def_t> map_cmdarg_def({
	{ "help"s, cmdarg_def_t({ "-h"s, "--help"s }, false) },
	{ "compile"s, cmdarg_def_t({ "-c"s, "--compile"s }, false) },
	{ "preprocess"s, cmdarg_def_t({ "-p"s, "--preprocess"s }, false) },
	{ "input-file"s, cmdarg_def_t({ "-i"s, "--input-file"s }, true) },
	{ "output-file"s, cmdarg_def_t({ "-o"s, "--output-file"s }, true) },
	{ "no-preprocess"s, cmdarg_def_t({ "-n"s, "--no-preprocess"s }, false) },
	});

/// <summary>Display help.</summary>
static void display_help() throw();
/// <summary>Command line action: Compile an MUAECL2 source file to a raw ECL file.</summary>
static void cmd_compile(unordered_map<string, cmdarg_input_t>& map_cmdarg_input);
/// <summary>Command line action: Preprocess (and not compile) an MUAECL2 source file to a preprocessed source file.</summary>
static void cmd_preprocess(unordered_map<string, cmdarg_input_t>& map_cmdarg_input);
static void preprocess(const PreprocessArguments& preprocess_args);
static void compile(const CompileArguments& compile_args);

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
				++it_vec_rawcmdarg;
				if (it_vec_rawcmdarg == vec_rawcmdarg.end()) throw(WrongCmdlineException("unexpected end of command line, expected value for argument "s + str_id));
				map_cmdarg_input[str_id].is_specified = true;
				map_cmdarg_input[str_id].value = *it_vec_rawcmdarg;
			} else {
				map_cmdarg_input[str_id].is_specified = true;
			}
		}

		if (map_cmdarg_input["help"s].is_specified) {
			display_help();
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
	}
	/*catch (ErrDesignApp &e) {
		cerr << e.what() << endl;
	}
	catch (exception &e) {
		cerr << e.what() << endl;
	}*/
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
  MUAECL2 {-c|--compile} [options]\n\
Compile an MUAECL2 source file to a raw ECL file.\n\
\n\
    {-i|--input-file} <input-filename>              The filename of the input file.\n\
    {-o|--output-file} <output-filename>            The filename of the output file.\n\
    {-n|--no-preprocess}                            Bypass the preprocessing step.\n\
\n\
  MUAECL2 {-p|--preprocess} [options]\n\
Preprocess (and not compile) an MUAECL2 source file to a preprocessed source file.\n\
\n\
    {-i|--input-file} <input-filename>              The filename of the input file.\n\
    {-o|--output-file} <output-filename>            The filename of the output file.\n\
\n\
Examples:\n\
  MUAECL2 -h\n\
  MUAECL2 -c -i st01.mecl -o st01.ecl\n\
  MUAECL2 -p -i st01.mecl -o st01p.mecl\n"s;
	cerr << str_help << endl;
}

static void cmd_compile(unordered_map<string, cmdarg_input_t>& map_cmdarg_input) {
	istream* in = nullptr;
	ostream* out = nullptr;

	unique_ptr<ifstream> in_f;
	if (map_cmdarg_input["input-file"s].is_specified) {
		in_f = unique_ptr<ifstream>(new ifstream(map_cmdarg_input["input-file"s].value));
		in = in_f.get();
	} else {
		in = &cin;
	}

	unique_ptr<ofstream> out_f;
	if (map_cmdarg_input["output-file"s].is_specified) {
		out_f = unique_ptr<ofstream>(new ofstream(map_cmdarg_input["output-file"s].value, ios_base::binary));
		out = out_f.get();
	} else {
		out = &cout;
	}


	if (map_cmdarg_input["no-preprocess"s].is_specified) {
		CompileArguments compile_args;
		compile_args.in = in;
		compile_args.out = out;
		compile(compile_args);
	} else {
		stringstream preprocessed;

		PreprocessArguments preprocess_args;
		preprocess_args.in = in;
		preprocess_args.out = &preprocessed;
		preprocess(preprocess_args);

		CompileArguments compile_args;
		compile_args.in = &preprocessed;
		compile_args.out = out;
		compile(compile_args);
	}
}

static void cmd_preprocess(unordered_map<string, cmdarg_input_t>& map_cmdarg_input) {
	PreprocessArguments preprocess_args;

	unique_ptr<ifstream> in_f;
	if (map_cmdarg_input["input-file"s].is_specified) {
		in_f = unique_ptr<ifstream>(new ifstream(map_cmdarg_input["input-file"s].value));
		preprocess_args.in = in_f.get();
	} else {
		preprocess_args.in = &cin;
	}

	unique_ptr<ofstream> out_f;
	if (map_cmdarg_input["output-file"s].is_specified) {
		out_f = unique_ptr<ofstream>(new ofstream(map_cmdarg_input["output-file"s].value));
		preprocess_args.out = out_f.get();
	} else {
		preprocess_args.out = &cout;
	}

	preprocess(preprocess_args);
}

static void preprocess(const PreprocessArguments& preprocess_args) {
	// TODO: Preprocess from *preprocess_args.in to *preprocess_args.out.
	while (!preprocess_args.in->eof()) {
		char c;
		preprocess_args.in->get(c);
		preprocess_args.out->put(c);
	}
}

static void compile(const CompileArguments& compile_args) {
	ReadIns::Read();
	Parser::initialize();
	//调试中
	Tokenizer tokenizer(*compile_args.in);
	Parser parser(tokenizer);
	tRoot* tree = parser.analyse();
	parser.TypeCheck();
	RawEclGenerator raw_ecl_generator(parser.Output());
	unique_ptr<ofstream> out_f;
	raw_ecl_generator.generate(*compile_args.out);
	Parser::clear();
}
