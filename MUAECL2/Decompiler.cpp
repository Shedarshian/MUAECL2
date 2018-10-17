#include "stdafx.h"
#include <string>
#include <list>
#include <map>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <sstream>
#include <algorithm>
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
		vector<dVar> variables;
		list<shared_ptr<DecodedSubDataEntry>> tree_original;
		/// Phase 0, handle variables declare
		{
			auto it_original = sub->data_entries.begin();
			if (sub->data_entries.size() != 0 && sub->data_entries[0]->data_entry_type == DecodedSubDataEntry::Ins) {
				/// extract the first ins, if it's ins40
				auto ptr = static_pointer_cast<DecodedIns>(sub->data_entries[0]);
				if (ptr->id == 40) {
					auto ptr2 = (*(ptr->params))[0];
					//need to be a int
					if (ptr2->param_type == DecodedParam::Int) {
						auto ptr3 = static_pointer_cast<DecodedParam_Int>(ptr2);
						variables.resize(ptr3->val / 4);
						for_each(variables.begin(), variables.end(), [c = "A"s, l = 0](dVar& v) mutable {
							v.name = c;
							int l2 = l;
							while (1) {
								if (c[l2] != 'Z') ++c[l2];
								else {
									c[l2] = 'A';
									if (l2 > 0) { --l2; continue; }
									else { ++l; c = 'A' + c; }
								}
								break;
							}
						});
						++it_original;
					}
				}
			}
			copy(it_original, sub->data_entries.end(), inserter(tree_original, tree_original.begin()));
		}
		//if the undecoration valid then assign type
		if (is_invalid) {
			if (variables.size() < types_param.size())
				//warning
				clog << "WARNING: MUAECL type function name found, but no enough space for local variable. Will discard the MUAECL decoration for function "s + name + "."s << endl;
			else
				for_each_n(variables.begin(), types_param.size(), [it = types_param.begin()](dVar& v) mutable {
					v.assign_type(*it); ++it;
				});
		}

		/// Phase A, translate operator and stackvar to expression, find and assign type for variable
		for (auto it = tree_original.begin(); it != tree_original.end(); ++it) {

		}
		/// Phase B, find jump pairs, in order to verify if/while/for
	}
}