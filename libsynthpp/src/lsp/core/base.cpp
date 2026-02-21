// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#include <lsp/core/base.hpp>

/// 型名をデマングルした文字列に変換します
std::string lsp::demangle(const char* mangled_name)
{
	std::string class_name;
#if defined(_MSC_VER)
	class_name = mangled_name;
	if(class_name.find("class ") != std::string::npos) {
		class_name = class_name.substr(6);
	} else if(class_name.find("struct ") != std::string::npos) {
		class_name = class_name.substr(7);
	}
#elif defined(__clang__)
	int stat;
	malloced_unique_ptr<char> name(abi::__cxa_demangle(mangled_name,0,0,&stat));
	if(name) {
		if(stat==0) {    // ステータスが0なら成功
			class_name = name.get();
		} else {
			// demangle失敗
			class_name = mangled_name;
		}
	} else {
		// demangle失敗
		class_name = mangled_name;
	}
#else
	class_name = v.name();
#endif
	return class_name;
}