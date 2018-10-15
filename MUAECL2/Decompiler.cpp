#include "stdafx.h"
#include <string>
#include <list>
#include <map>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <sstream>
#include "Decompiler.h"
#include "NameDecorator.h"
#include "RawEclDecoder.h"

using namespace std;

Decompiler::Decompiler() {}

Decompiler::~Decompiler() {}

void Decompiler::DecompileRoot(istream& instream, ostream& outstream) {
	auto root = RawEclDecoder().DecodeRawEclRoot(instream);

	regex rgx_find_quote("\\\"", regex::flag_type::optimize);
	string str_rpl_quote("\\\"");
	for (const string& val_anim : root->anim) {
		outstream << "#anim \"" << regex_replace(val_anim, rgx_find_quote, str_rpl_quote, regex_constants::match_flag_type::match_default) << "\"\n";
	}
	for (const string& val_ecli : root->ecli) {
		outstream << "#ecli \"" << regex_replace(val_ecli, rgx_find_quote, str_rpl_quote, regex_constants::match_flag_type::match_default) << "\"\n";
	}
	outstream << "\n";

	int indent_level = 0;
	for (auto sub : root->subs) {
		Op::mType type_ret;
		vector<Op::mType> types_param;
		bool is_invalid;
		string name = NameDecorator::undecorateSubName(sub->id, type_ret, types_param, is_invalid);
		/// TODO: variables declare
		vector<dVar> variables;

		list<shared_ptr<DecodedSubDataEntry>> tree_original;
		copy(sub->data_entries.begin(), sub->data_entries.end(), inserter(tree_original, tree_original.begin()));

		/// Phase A, translate operator and stackvar to expression, find and assign type for variable
		for (auto it = tree_original.begin(); it != tree_original.end(); ++it) {

		}
		/// Phase B, find jump pairs, in order to verify if/while/for
	}
}