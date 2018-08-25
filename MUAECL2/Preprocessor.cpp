#include "stdafx.h"
#include "Preprocessor.h"
#include "Misc.h"
#include <list>
#include <iostream>
#include <string>
#include <regex>
#include <memory>
#include <utility>
#include <map>
using namespace std;
#define CAP_DEFAULT 128

pair<vector<string>, vector<string>> Preprocessor::process(istream &in, ostream &out) {
	vector<string> ecli, anim;
	string v;
	int lineNo = 1;
	list<tuple<string, regex, string>> subs;	//name and regex and replace_string
	while (!in.eof()) {
		int blankline = 0;
		while (1) {
			int size_old = v.size(), size = 0;
			while (1) {
				v.resize(size_old + size + CAP_DEFAULT + 1);
				in.getline(&v[size_old + size], CAP_DEFAULT + 1);
				if (in.rdstate() & ios_base::failbit && !in.eof()) {
					in.clear();
					size += CAP_DEFAULT;
				}
				else {
					v.resize(size_old + size + in.gcount() - (in.eof() ? 0 : 1));
					break;
				}
			}
			if (v.length() > 0 && v.back() == '\\') {
				v.erase(v.end() - 1);
				++blankline;
			}
			else
				break;
		}
		if (v != "") {
			v = v.substr(v.find_first_not_of(" \t"));
			if (v.front() != '#') {
				for (auto it = subs.crbegin(); it != subs.crend(); ++it)
					v = regex_replace(v, get<1>(*it), get<2>(*it));
				for (auto it = ReadIns::include.crbegin(); it != ReadIns::include.crend(); ++it)
					v = regex_replace(v, it->first, it->second);
				out << v << '\n';
			}
			else {
				v.erase(0, 1);
				if (v[0] == 's' && (v[1] < 'a' || v[1] > 'z')) {
					//#s deliminator pattern deliminator string
					char deliminator = v[1];
					if (deliminator >= 'A' && deliminator <= 'Z' || deliminator >= '0' && deliminator <= '9' || deliminator == '_')
						throw(ErrSubstituteDeliminator(lineNo, deliminator, false));
					int delim2 = v.find_first_of(deliminator, 2);
					if (delim2 == -1)
						throw(ErrSubstituteNoString(lineNo));
					int delim3 = v.find_first_of(deliminator, delim2 + 1);
					string pattern = v.substr(2, delim2 - 2);
					string replace_string = v.substr(delim2 + 1, delim3 - delim2 - 1);
					string name = (delim3 == -1) ? (to_string(lineNo) + "\n") : v.substr(delim3 + 1);
					if (delim3 != -1 && !regex_match(name, regex("[_[:alnum:]]+")))
						throw(ErrSubstituteName(lineNo, name));
					if (find_if(subs.begin(), subs.end(), [&name](tuple<string, regex, string> t) { return get<0>(t) == name; }) != subs.end())
						throw(ErrSubstituteRedeclare(lineNo, name));
					subs.emplace_back(name, regex(pattern), replace_string);
				}
				else if (v.compare(0, 7, "define ") == 0) {
					//#define space identifier space string
					//#define space identifier ( identifier_list ) space string
					int delim2 = v.find_first_of(" {", 7);
					string identifier = v.substr(7, delim2 - 7);
					if (!regex_match(identifier, regex("[_[:alpha:]][_[:alnum:]]*")))
						throw(ErrSubstituteName(lineNo, identifier));
					if (v[delim2] == ' ') {
						string replace_string = v.substr(delim2 + 1);
						if (find_if(subs.begin(), subs.end(), [&identifier](tuple<string, regex, string> t) { return get<0>(t) == identifier; }) != subs.end())
							throw(ErrSubstituteRedeclare(lineNo, identifier));
						subs.emplace_back(identifier, regex("\\b" + identifier + "\\b"), replace_string);
					}
					else {
						int delim3 = v.find_first_of('}', 7);
						string replace_string = v.substr(delim3 + 2);
						string identifier_list = v.substr(delim2 + 1, delim3 - delim2 - 1);
						if (!regex_match(identifier_list, regex("|\\s*[_[:alpha:]][_[:alnum:]]*(\\s*,\\s*[_[:alpha:]][_[:alnum:]]*)*\\s*")))
							throw(ErrDeclareIdList(lineNo, identifier_list));
						if (identifier_list != "") {
							regex word = regex("[_[:alpha:]][_[:alnum:]]*");
							auto it_begin = sregex_iterator(identifier_list.begin(), identifier_list.end(), word);
							auto it_end = sregex_iterator();
							string identifier_replace_list;
							int n = 1;
							for (auto it = it_begin; it != it_end; ++it, ++n) {
								char c[4];
								snprintf(c, 4, "$%.2d", n);
								replace_string = regex_replace(replace_string, regex((*it).str()), c, regex_constants::format_sed);
								identifier_replace_list += "\\s*(.*?\\S)\\s*,";
							}
							identifier_replace_list.erase(identifier_replace_list.end() - 1);
							subs.emplace_back(identifier, regex("\\b" + identifier + "\\s*\\{" + identifier_replace_list + "\\}"), replace_string);
						}
						else {
							subs.emplace_back(identifier, regex("\\b" + identifier + "\\s*\\{\\s*\\}"), replace_string);
						}
					}
				}
				else if (v.compare(0, 4, "ends") == 0) {
					//#ends deliminator name
					char deliminator = v[4];
					if (deliminator >= 'a'&&deliminator <= 'z' || deliminator >= 'A' && deliminator <= 'Z' || deliminator >= '0' && deliminator <= '9' || deliminator == '_')
						throw(ErrSubstituteDeliminator(lineNo, deliminator, true));
					string name = v.substr(5);
					auto it = find_if(subs.begin(), subs.end(), [&name](tuple<string, regex, string> t) { return get<0>(t) == name; });
					if (it == subs.end())
						throw(ErrEndsNotFound(lineNo, name, "ends"));
					subs.erase(it);
				}
				else if (v.compare(0, 6, "undef ") == 0) {
					//#undef space name
					string name = v.substr(6);
					auto it = find_if(subs.begin(), subs.end(), [&name](tuple<string, regex, string> t) { return get<0>(t) == name; });
					if (it == subs.end())
						throw(ErrEndsNotFound(lineNo, name, "undef"));
					subs.erase(it);
				}
				else if (v.compare(0, 5, "ecli ") == 0) {
					//#ecli space name
					ecli.push_back(v.substr(5));
				}
				else if (v.compare(0, 5, "anim ") == 0) {
					//#anim space name
					anim.push_back(v.substr(5));
				}
				else {
					throw(ErrPreprocessNotFound(lineNo, v));
				}
				out << '\n';
			}
		}
		else {
			out << '\n';
		}
		lineNo += blankline + 1;
		for (; blankline > 0; --blankline)
			out << '\n';
		v.clear();
	}
	return make_pair(ecli, anim);
}
