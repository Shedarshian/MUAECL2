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

	/// <summary>Get a type from its decorated name.</summary>
	/// <param name="decorated_name">
	/// Pointer to a string that contains the decorated name.
	/// On calling, the caller initializes this parameter so that it points to the start of the decorated name of the type.
	/// On return, the parameter points to just after the end of the decorated name of the type.
	/// </param>
	/// <param name="decorated_name_end">Pointer to just after the end of the string that contains the decorated name.</param>
	/// <param name="is_invalid_decorated_name">
	/// The function sets this parameter to true if it failed to undecorate the name.
	/// Otherwise, it leaves this parameter unchanged.
	/// </param>
	/// <returns>The type.</returns>
	Op::mType undecorateTypeName(const char*& decorated_name, const char* decorated_name_end, bool& is_invalid_decorated_name);

	/// <summary>Generate decorated name for a subroutine.</summary>
	/// <param name="name">The name (in the source code) of the subroutine.</param>
	/// <param name="type_ret">The return type of the subroutine.</param>
	/// <param name="types_param">The types of the formal parameters, from left to right.</param>
	/// <returns>The decorated name.</returns>
	string decorateSubName(const string& name, Op::mType type_ret, const vector<Op::mType>& types_param);

	/// <summary>Get original name and type information of a subroutine from its decorated name.</summary>
	/// <param name="decorated_name">The decorated name.</param>
	/// <param name="type_ret">The return type of the subroutine.</param>
	/// <param name="types_param">The types of the formal parameters, from left to right.</param>
	/// <param name="is_invalid_decorated_name">
	/// The function sets this parameter to true if it failed to undecorate the name.
	/// Otherwise, it leaves this parameter unchanged.
	/// </param>
	/// <returns>The name (in the source code) of the subroutine.</returns>
	string undecorateSubName(const string& decorated_name, Op::mType& type_ret, vector<Op::mType>& types_param, bool& is_invalid_decorated_name);
}
