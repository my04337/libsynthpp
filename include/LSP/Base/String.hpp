#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP
{

using char_t = wchar_t;
using string_t = std::wstring;
using ostream_t = std::wostream;
using ostringstream_t = std::wostringstream;

// 文字列操作クラス
class strings
{
public:
	strings() = delete;
	
	static string_t to_string(const char* s);
	static string_t to_string(const char* s, size_t len);
	static string_t to_string(const std::string& str);

	static string_t replace_all(const string_t& str, char_t from, char_t to);
};

}