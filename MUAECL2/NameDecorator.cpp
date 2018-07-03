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
		string name_decorated("@MUAECL2Sub@"s + name + "@"s + decorateTypeName(type_ret) + "@"s);
		for (Op::mType val_types_param : types_param) {
			name_decorated += decorateTypeName(val_types_param);
		}
		name_decorated += "@"s;
		return name_decorated;
	}

	string decorateParamVarName(size_t i_param, Op::mType type) {
		stringstream sstream_i_param;
		sstream_i_param << i_param;
		return "@MUAECL2Var@P@"s + sstream_i_param.str() + "@"s + decorateTypeName(type) + "@"s;
	}

	string decorateDeclVarName(const mVar& var) {
		return "@MUAECL2Var@D@"s + var.id + "@"s + decorateTypeName(var.type) + "@"s;
	}
}