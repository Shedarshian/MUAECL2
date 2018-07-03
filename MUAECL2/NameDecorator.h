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

	/// <summary>Generate decorated variable name for a formal parameter variable.</summary>
	/// <param name="i_param">The 0-based index of the formal parameter, from left to right.</param>
	/// <param name="type">The type of the parameter.</param>
	/// <returns>The decorated name.</returns>
	string decorateParamVarName(size_t i_param, Op::mType type);

	/// <summary>Generate decorated variable name for a declared variable.</summary>
	/// <param name="var">The declared variable.</param>
	/// <returns>The decorated name.</returns>
	string decorateDeclVarName(const mVar& var);
}