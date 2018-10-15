#pragma once
#include <iostream>
#include "Misc.h"
#include "RawEclDecoder.h"

using namespace std;

/// <summary>An MUAECL Decompiler that converts decoded raw ECL to MUAECL code.</summary>
class Decompiler final {
public:
	Decompiler();
	~Decompiler();
	void DecompileRoot(istream& instream, ostream& outstream);
private:

};
