// MUAECL2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "RawEclGenerator.h"
#include "RawEclDecoder.h"
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
	Parser::initialize();
	ReadIns::Read();
	try {
		//调试中
		ifstream in(argv[1]);
		//ifstream in("D:\\A.学习【Learning】\\A2.Mathematics\\VC\\C++workspace\\MUAECL2\\in.txt");
		Tokenizer tokenizer(in);
		Parser parser(tokenizer);
		tRoot* tree = parser.analyse();
		parser.TypeCheck();
		RawEclGenerator raw_ecl_generator(parser.Output());
		raw_ecl_generator.generate(cout);
	}
	catch (ExceptionWithLineNo &e) {
		cerr << e.lineNo << " " << e.what() << endl;
	}
	catch (DecoderException &e) {
		char str_offs[1024];
		sprintf_s(str_offs, 1024, "0x%08zX", e.GetOffset());
		cerr << "Decoder : 0x" << str_offs << " : " << e.what() << endl;
	}
	catch (ErrDesignApp &e) {
		cerr << e.what() << endl;
	}
	catch (exception &e) {
		cerr << e.what() << endl;
	}
	Parser::clear();
    return 0;
}

