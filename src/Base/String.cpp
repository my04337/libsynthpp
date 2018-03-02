#include <LSP/Base/String.hpp>

using namespace LSP;

string_t strings::to_string(const std::string& str)
{
	if(str.empty()) return {};

	string_t ret;

	const char* s = str.c_str();

	mbstate_t st;
	memset(&st, 0, sizeof(st));


	while (true) {
		constexpr size_t buffLen = 64;
		wchar_t buff[buffLen+1] = L"";

		size_t result = mbsrtowcs(buff, &s, buffLen, &st);
		if(result == 0) {
			// 変換完了 (もう変換できるものがない)
			break;
		} else if (result != static_cast<size_t>(-1)) {
			// result文字分変換
			ret.append(buff);
		} else {
			// 変換エラー : 読み飛ばし & 読めなかったことを示す文字に置き換え
			ret.append(L"?");
			++s;
			if(s - str.c_str() >= str.size()) break;
		}
	}

	return ret;
}
string_t strings::to_string(const char* s)
{
	if(!s) return {};
	return to_string(std::string(s));
}
string_t strings::to_string(const char* s, size_t len)
{
	if(!s || !len) return {};
	return to_string(std::string(s, len));
}
string_t strings::replace_all(const string_t& str_, char_t from, char_t to)
{
	string_t str = str_;
	for (auto& ch : str) {
		if(ch == from) ch = to;
	}
	return str; // NRVO
}