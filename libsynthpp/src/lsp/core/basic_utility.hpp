// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT-0

// Basic Utilities (MIT-0)
// 
// This file contains general-purpose utility primitives.
// Licensed under MIT-0: no attribution or copyright notice is required.
// You are free to use, modify, and redistribute this file without restriction.
//
// 汎用的なユーティリティをまとめたヘッダです。
// MIT-0ライセンスにより提供されており、著作権表示や帰属表示は不要です。
// 自由に利用・改変・再配布いただけます。


#pragma once

#include <any>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

namespace lsp
{

/// コピー禁止,ムーブ可能型
struct non_copy {
	constexpr non_copy() = default;
	non_copy(const non_copy&) = delete;
	non_copy(non_copy&&)noexcept = default;
	non_copy& operator=(const non_copy&) = delete;
	non_copy& operator=(non_copy&&)noexcept = default;
};

/// コピー,ムーブ禁止型
struct non_copy_move {
	constexpr non_copy_move() = default;
	non_copy_move(const non_copy_move&) = delete;
	non_copy_move(non_copy_move&&)noexcept = delete;
	non_copy_move& operator=(const non_copy_move&) = delete;
	non_copy_move& operator=(non_copy_move&&)noexcept = delete;
};

// テンプレート補助 : どのような値を受け取ってもfalseを表す値
template<typename ...>
constexpr bool false_v = false;

/// スコープ離脱時実行コード 補助クラス
template<typename F>
class [[nodiscard]] _finally_action
	: non_copy
{
public:
	_finally_action() : f(), valid(false) {}
	_finally_action(F f) : f(std::move(f)), valid(true) {}
	~_finally_action() { action(); }

	_finally_action(_finally_action&& d)noexcept : f(std::move(d.f)), valid(true) { d.valid=false; };
	_finally_action& operator=(_finally_action&& d)noexcept { reset(); f = std::move(d.f); valid=d.valid; d.valid=false; return *this;}

	void action()noexcept { if(valid) f(); reset(); }
	void reset()noexcept { valid = false; }

private:
	F f;
	bool valid;
};

/// スコープ離脱時実行コード 定義関数
template<typename F>
_finally_action<F> finally(F&& f) { return {std::forward<F>(f)}; }

/// 型名をデマングルした文字列に変換します
std::string demangle(const char* mangled_name);
inline std::string demangle(const std::type_info& v) { return lsp::demangle(v.name()); }
inline std::string demangle(const std::type_index& v) { return lsp::demangle(v.name()); }

// C++20 透過的ハッシュ用関数オブジェクト
// https://cpprefjp.github.io/reference/functional/hash.html
struct string_hash 
{
	using is_transparent = void;
	// string/string_view/const char*共用ハッシュ計算
	size_t operator()(std::string_view sv) const {
		return std::hash<std::string_view>{}(sv);
	}
};

// std::anyアクセス用便利関数
template <class map_type, class key_type, class value_type>
auto get_any_or(map_type& map, key_type&& key, value_type&& defaultValue) 
{
	if(const auto it = map.find(key); it != map.end() && it->second.has_value()) {
		return std::any_cast<std::decay_t<value_type>>(it->second);
	}
	else {
		return std::forward<value_type>(defaultValue);
	}
};

} // namespace lsp
