#include "stdafx.h"
#include <string>
#include <memory>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include "Misc3.h"

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")

using namespace std;

wstring u8string_to_u16string(const string& u8str) {
	size_t cch_u16str = MultiByteToWideChar(CP_UTF8, 0, u8str.data(), u8str.size(), nullptr, 0);
	if (cch_u16str) {
		unique_ptr<wchar_t[]> u16str(new wchar_t[cch_u16str + 1]);
		if (!MultiByteToWideChar(CP_UTF8, 0, u8str.data(), u8str.size(), u16str.get(), cch_u16str + 1)) throw(ErrExternalAPIError("MultiByteToWideChar"));
		return wstring(u16str.get(), cch_u16str);
	} else {
		return wstring();
	}
}

string u16string_to_u8string(const wstring& u16str) {
	size_t cb_u8str = WideCharToMultiByte(CP_UTF8, 0, u16str.data(), u16str.size(), nullptr, 0, nullptr, nullptr);
	if (cb_u8str) {
		unique_ptr<char[]> u8str(new char[cb_u8str + 1]);
		if (!WideCharToMultiByte(CP_UTF8, 0, u16str.data(), u16str.size(), u8str.get(), cb_u8str + 1, nullptr, nullptr)) throw(ErrExternalAPIError("WideCharToMultiByte"));
		return string(u8str.get(), cb_u8str);
	} else {
		return string();
	}
}

string base64_encode_string(const string& str) {
	DWORD cch_buf_out = 0;
	CryptBinaryToStringW(reinterpret_cast<const BYTE*>(str.c_str()), str.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &cch_buf_out);
	unique_ptr<wchar_t[]> wstrbuf_out(new wchar_t[cch_buf_out]);
	if (!CryptBinaryToStringW(reinterpret_cast<const BYTE*>(str.c_str()), str.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, wstrbuf_out.get(), &cch_buf_out)) throw(ErrExternalAPIError("CryptBinaryToStringW"));
	return u16string_to_u8string(wstring(wstrbuf_out.get(), cch_buf_out));
}

string base64_decode_string(const string& str) {
	wstring wstr_in(u8string_to_u16string(str));
	DWORD cb_buf_out = 0;
	CryptStringToBinaryW(wstr_in.c_str(), wstr_in.size(), CRYPT_STRING_BASE64, nullptr, &cb_buf_out, nullptr, nullptr);
	unique_ptr<BYTE[]> buf_out(new BYTE[cb_buf_out]);
	if (!CryptStringToBinaryW(wstr_in.c_str(), wstr_in.size(), CRYPT_STRING_BASE64, buf_out.get(), &cb_buf_out, nullptr, nullptr)) throw(ErrExternalAPIError("CryptStringToBinaryW"));
	return string(reinterpret_cast<char*>(buf_out.get()), cb_buf_out);
}

string hash_string(const string& str) {
	struct halg_t {
		BCRYPT_ALG_HANDLE halg = nullptr;
		~halg_t() {
			if (this->halg) {
				BCryptCloseAlgorithmProvider(this->halg, 0);
			}
		}
	} halg;
	if (BCryptOpenAlgorithmProvider(&halg.halg, BCRYPT_SHA256_ALGORITHM, nullptr, 0)) throw(ErrExternalAPIError("BCryptOpenAlgorithmProvider"));
	{
		unsigned long cb_hashobj = 0;
		{
			unsigned long cb_res = 0;
			if (BCryptGetProperty(halg.halg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<unsigned char*>(&cb_hashobj), sizeof(unsigned long), &cb_res, 0)) throw(ErrExternalAPIError("BCryptGetProperty"));
		}
		unique_ptr<unsigned char[]> hashobj(new unsigned char[cb_hashobj]);
		struct hhash_t {
			BCRYPT_HASH_HANDLE hhash = nullptr;
			~hhash_t() {
				if (this->hhash) {
					BCryptDestroyHash(this->hhash);
				}
			}
		} hhash;
		if (BCryptCreateHash(halg.halg, &hhash.hhash, hashobj.get(), cb_hashobj, nullptr, 0, 0)) throw(ErrExternalAPIError("BCryptCreateHash"));
		unique_ptr<unsigned char[]> buf_data(new unsigned char[str.size()]);
		memcpy(buf_data.get(), str.data(), str.size());
		if (BCryptHashData(hhash.hhash, buf_data.get(), str.size(), 0)) throw(ErrExternalAPIError("BCryptHashData"));
		unsigned long cb_hash = 0;
		{
			unsigned long cb_res = 0;
			if (BCryptGetProperty(halg.halg, BCRYPT_HASH_LENGTH, reinterpret_cast<unsigned char*>(&cb_hash), sizeof(unsigned long), &cb_res, 0)) throw(ErrExternalAPIError("BCryptGetProperty"));
		}
		unique_ptr<unsigned char[]> buf_hash(new unsigned char[cb_hash]);
		if (BCryptFinishHash(hhash.hhash, buf_hash.get(), cb_hash, 0)) throw(ErrExternalAPIError("BCryptFinishHash"));
		return string(reinterpret_cast<const char*>(buf_hash.get()), cb_hash);
	}
}