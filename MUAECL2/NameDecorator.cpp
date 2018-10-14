#include "stdafx.h"
#include <unordered_map>
#include <regex>
#include "NameDecorator.h"

using namespace std;

namespace NameDecorator {
	class ErrInvalidDecoratedName : exception {
	public:
		ErrInvalidDecoratedName() : exception("invalid decorated name") {}
	};

	string decorateTypeName(Op::mType type) {
		switch (type) {
		case Op::mType::type_error:
			throw(ErrDesignApp("NameDecorator::decorateTypeName(Op::mType::type_error) not permitted"));
		case Op::mType::Void:
			return "0"s;
		case Op::mType::Int:
			return "A"s;
		case Op::mType::Float:
			return "B"s;
		case Op::mType::Point:
			return "C"s;
		case Op::mType::String:
			return "D"s;
		case Op::mType::inilist:
			return "E"s;
		default:
			throw(ErrDesignApp("unknown mType when calling decorateTypeName"));
		}
	}

	Op::mType undecorateTypeName(const char*& decorated_name, const char* decorated_name_end, bool& is_invalid_decorated_name) {
#ifdef MUAECL2_NO_DECORATE
		is_invalid_decorated_name = true;
		return Op::mType::type_error;
#else
		if (!decorated_name) throw(ErrDesignApp("NameDecorator::undecorateTypeName : !decorated_name"));
		if (!decorated_name_end) throw(ErrDesignApp("NameDecorator::undecorateTypeName : !decorated_name_end"));
		Op::mType type = Op::mType::type_error;
		try {
			static const unordered_map<char, Op::mType> map_type_simple(
				{ { '0', Op::mType::Void },
				{ 'A', Op::mType::Int },
				{ 'B', Op::mType::Float },
				{ 'C', Op::mType::Point },
				{ 'D', Op::mType::String },
				{ 'E', Op::mType::inilist } }
			);
			if (decorated_name == decorated_name_end) throw(ErrInvalidDecoratedName());
			try {
				Op::mType type = map_type_simple.at(*decorated_name);
			} catch (out_of_range&) {}
			if (type != Op::mType::type_error) {
				++decorated_name;
			} else {
				throw(ErrInvalidDecoratedName());
			}
		} catch (ErrInvalidDecoratedName&) {
			is_invalid_decorated_name = true;
			return Op::mType::type_error;
		}
		return type;
#endif
	}

	string decorateSubName(const string& name, Op::mType type_ret, const vector<Op::mType>& types_param) {
#ifdef MUAECL2_NO_DECORATE
		return name;
#else
		string name_decorated("@MUAECL2Sub@"s + name + "@"s + decorateTypeName(type_ret) + "@"s);
		for (Op::mType val_types_param : types_param) {
			name_decorated += decorateTypeName(val_types_param);
		}
		name_decorated += "@"s;
		return name_decorated;
#endif
	}

	string undecorateSubName(const string& decorated_name, Op::mType& type_ret, vector<Op::mType>& types_param, bool& is_invalid_decorated_name) {
#ifdef MUAECL2_NO_DECORATE
		is_invalid_decorated_name = true;
		return decorated_name;
#else
		string name;
		try {
			const char* pstr_decorated_name = decorated_name.c_str();
			const char* pstr_decorated_name_end = decorated_name.c_str() + decorated_name.size();
			{
				static const char decorated_name_prefix[] = "@MUAECL2Sub@";
				size_t i;
				for (i = 0; pstr_decorated_name != pstr_decorated_name_end && i < sizeof(decorated_name_prefix) - 1; ++pstr_decorated_name, ++i) {
					if (*pstr_decorated_name != decorated_name_prefix[i]) break;
				}
				if (i < sizeof(decorated_name_prefix) - 1) throw(ErrInvalidDecoratedName());
			}
			{
				static const regex rgx_identifier(""s, regex::flag_type::optimize);
				for (; pstr_decorated_name != pstr_decorated_name_end && *pstr_decorated_name != '@'; ++pstr_decorated_name) {
					name.push_back(*pstr_decorated_name);
				}
				if (name.empty()) throw(ErrInvalidDecoratedName());
				if (!regex_match(name, rgx_identifier)) throw(ErrInvalidDecoratedName());
			}
			{
				if (pstr_decorated_name == pstr_decorated_name_end || *pstr_decorated_name != '@') throw(ErrInvalidDecoratedName());
				++pstr_decorated_name;
			}
			{
				type_ret = undecorateTypeName(pstr_decorated_name, pstr_decorated_name_end, is_invalid_decorated_name);
				if (is_invalid_decorated_name) throw(ErrInvalidDecoratedName());
			}
			{
				if (pstr_decorated_name == pstr_decorated_name_end || *pstr_decorated_name != '@') throw(ErrInvalidDecoratedName());
				++pstr_decorated_name;
			}
			{
				types_param.clear();
				while (pstr_decorated_name != pstr_decorated_name_end && *pstr_decorated_name != '@') {
					types_param.push_back(undecorateTypeName(pstr_decorated_name, pstr_decorated_name_end, is_invalid_decorated_name));
					if (is_invalid_decorated_name) throw(ErrInvalidDecoratedName());
				}
			}
			{
				if (pstr_decorated_name == pstr_decorated_name_end || *pstr_decorated_name != '@') throw(ErrInvalidDecoratedName());
				++pstr_decorated_name;
			}
			if (pstr_decorated_name != pstr_decorated_name_end) throw(ErrInvalidDecoratedName());
		} catch (ErrInvalidDecoratedName&) {
			is_invalid_decorated_name = true;
			return decorated_name;
		}
		return name;
#endif
	}
}
