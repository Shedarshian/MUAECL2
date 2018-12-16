#pragma once
#include <string>

using namespace std;

class ErrExternalAPIError :public exception {
public:
	ErrExternalAPIError(const char* what) : s("Failed to call external API "s + what + "!"s) {}
	ErrExternalAPIError(const string what) : s("Failed to call external API "s + what + "!"s) {}
	virtual const char* what() const throw() { return s.c_str(); }
private:
	const string s;
};

wstring u8string_to_u16string(const string& u8str);
string u16string_to_u8string(const wstring& u16str);
string base64_encode_string(const string& str);
string base64_decode_string(const string& str);
string hash_string(const string& str);
