// MUAECL2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Preprocessor.h"
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
#include <map>

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
	/// <param name="">Whether this argument has a value (like "-o"), or is a command line switch.</param>
	cmdarg_def_t(bool is_value_arg) : is_value_arg(is_value_arg) {}
	bool isValueArg() const { return this->is_value_arg; }
private:
	bool is_value_arg = false;
};

/// <summary>Parsed arguments passed to the compiling routine.</summary>
struct CompileArguments final {
	string filename_in;
	string filename_out;
};

/// <summary>Command line argument definition map.
/// From the identifier of an argument to the <c>cmdarg_def_t</c> object that contains its definition.
/// </summary>
static const map<string, cmdarg_def_t> map_cmdarg_def({
	{ "-h"s, cmdarg_def_t(false) },
	{ "--help"s, cmdarg_def_t(false) },
	{ "-c"s, cmdarg_def_t(false) },
	{ "--compile"s, cmdarg_def_t(false) },
	{ "-i"s, cmdarg_def_t(true) },
	{ "--input-file"s, cmdarg_def_t(true) },
	{ "-o"s, cmdarg_def_t(true) },
	{ "--output-file"s, cmdarg_def_t(true) }
	});

/// <summary>Display help.</summary>
static void display_help() throw();
/// <summary>Compile an MUAECL2 source file to a raw ECL file.</summary>
static void compile(const CompileArguments& compile_args);

int main(int argc, char* argv[]) {
	try {
		string v("define abc(d, e, f) def");
		int delim2 = v.find_first_of(" (", 7);
		string identifier = v.substr(7, delim2 - 7);
		int delim3 = v.find_first_of(')', 7);
		string replace_string = v.substr(delim3 + 2);
		string identifier_list = v.substr(delim2 + 1, delim3 - delim2 - 1);
		cout << identifier << endl << identifier_list << endl << replace_string << endl;

		vector<string> vec_rawcmdarg;
		for (int i = 1; i < argc; ++i) {
			vec_rawcmdarg.emplace_back(argv[i]);
		}

		struct cmdarg_input_t {
			bool is_specified = false;
			string value;
		};
		map<string, cmdarg_input_t> map_cmdarg_input;

		for (
			vector<string>::const_iterator it_vec_rawcmdarg = vec_rawcmdarg.begin();
			it_vec_rawcmdarg != vec_rawcmdarg.end();
			++it_vec_rawcmdarg
			) {
			string str_id = *it_vec_rawcmdarg;
			const cmdarg_def_t* cmdarg_def = nullptr;
			try {
				cmdarg_def = &map_cmdarg_def.at(str_id);
			} catch (out_of_range&) {
				throw(WrongCmdlineException("unknown command line argument: "s + str_id));
			}
			if (cmdarg_def->isValueArg()) {
				++it_vec_rawcmdarg;
				if (it_vec_rawcmdarg == vec_rawcmdarg.end()) throw(WrongCmdlineException("unexpected end of command line, expected value for "s + str_id));
				map_cmdarg_input[str_id].is_specified = true;
				map_cmdarg_input[str_id].value = *it_vec_rawcmdarg;
			} else {
				map_cmdarg_input[str_id].is_specified = true;
			}
		}

		if (map_cmdarg_input["-h"].is_specified || map_cmdarg_input["--help"].is_specified) {
			display_help();
		} else if (map_cmdarg_input["-c"].is_specified || map_cmdarg_input["--compile"].is_specified) {
			CompileArguments compile_args;
			if (map_cmdarg_input["-i"].is_specified) compile_args.filename_in = map_cmdarg_input["-i"].value;
			if (map_cmdarg_input["--input-file"].is_specified) compile_args.filename_in = map_cmdarg_input["--input-file"].value;
			if (map_cmdarg_input["-o"].is_specified) compile_args.filename_out = map_cmdarg_input["-o"].value;
			if (map_cmdarg_input["--output-file"].is_specified) compile_args.filename_out = map_cmdarg_input["--output-file"].value;
			compile(compile_args);
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
\n\
Examples:\n\
  MUAECL2 -h\n\
  MUAECL2 -c -i st01.mecl -o st01.ecl\n\
		"s;
	cerr << str_help << endl;
}

static void compile(const CompileArguments& compile_args) {
	ReadIns::Read();
	Parser::initialize();
	//调试中
	unique_ptr<ifstream> in_f;
	istream* in = nullptr;
	if (compile_args.filename_in.empty()) {
		in = &cin;
	} else {
		in_f = unique_ptr<ifstream>(new ifstream(compile_args.filename_in));
		in = in_f.get();
	}
	stringstream preprocess_out;			//if preprocess no output to file
	Preprocessor::process(*in, preprocess_out);
	Tokenizer tokenizer(preprocess_out);
	Parser parser(tokenizer);
	tRoot* tree = parser.analyse();
	parser.TypeCheck();
	RawEclGenerator raw_ecl_generator(parser.Output());
	unique_ptr<ofstream> out_f;
	if (!compile_args.filename_out.empty()) {
		ofstream out(compile_args.filename_out, ios::binary);
		raw_ecl_generator.generate(out);
	}
	Parser::clear();
}
