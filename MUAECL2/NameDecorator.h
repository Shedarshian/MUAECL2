#pragma once
#include <string>
#include <vector>
#include "Misc.h"
#include "GrammarTree.h"

using namespace std;

namespace NameDecorator {
	/// <summary>Generate decorated name for a type.</summary>
	/// <param name="type">The type.</param>
	/// <returns>The decorated name.</returns>
	string decorateTypeName(Op::mType type);

	/// <summary>Generate decorated name for a subroutine.</summary>
	/// <param name="name">The name (in the source code) of the subroutine.</param>
	/// <param name="type_ret">The return type of the subroutine.</param>
	/// <param name="types_param">The types of the formal parameters, from left to right.</param>
	/// <returns>The decorated name.</returns>
	string decorateSubName(const string& name, Op::mType type_ret, const vector<Op::mType>& types_param);
}