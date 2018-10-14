#include "stdafx.h"
#include <string>
#include <list>
#include <map>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <sstream>
#include "Decompiler.h"
#include "RawEclDecoder.h"

using namespace std;

Decompiler::Decompiler() {}

Decompiler::~Decompiler() {}

void Decompiler::DecompileRoot(const DecodedRoot& root, ostream& stream) {
	regex rgx_find_quote("\\\"", regex::flag_type::optimize);
	string str_rpl_quote("\\\"");
	for (const string& val_anim : root.anim) {
		stream << "#anim \"" << regex_replace(val_anim, rgx_find_quote, str_rpl_quote, regex_constants::match_flag_type::match_default) << "\"\n";
	}
	for (const string& val_ecli : root.anim) {
		stream << "#ecli \"" << regex_replace(val_ecli, rgx_find_quote, str_rpl_quote, regex_constants::match_flag_type::match_default) << "\"\n";
	}
	stream << "\n";
}