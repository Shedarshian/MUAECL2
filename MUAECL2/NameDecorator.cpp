#include "stdafx.h"
#include <sstream>
#include "NameDecorator.h"

using namespace std;

namespace NameDecorator {
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
}