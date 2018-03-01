#pragma once

// --- 基本的なヘッダ類 ---
#include <list>
#include <vector>
#include <chrono>
#include <limits>
#include <memory>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <functional>
#include <typeinfo>
#include <typeindex>

// --- 基本的なマクロ類 ---
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 
#define STRICT 
#define NOMINMAX 
#endif

// マクロ展開遅延
#define DELAY_MACRO(...)  DELAY_MACRO_(__VA_ARGS__)
#define DELAY_MACRO_(...) __VA_ARGS__

// --- 基本的な型 ---
namespace LSP
{
// 標準的な浮動小数点数型 : float32を使用
using float_t = float; 

// 円周率
constexpr float_t PI = 3.14159265f;

}

// --- 基本的なクラス ---
namespace LSP
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

/// スコープ離脱時実行コード 補助クラス
template<typename F>
class _finally_action
	: non_copy
{
public:
	_finally_action() : f(), valid(false) {}
	_finally_action(F f) : f(std::move(f)), valid(true) {}
	~_finally_action() { action(); }

	_finally_action(_finally_action&& d) : f(std::move(d.f)), valid(true) { d.valid=false; };
	_finally_action& operator=(_finally_action&& d) { reset(); f = std::move(d.f); valid=d.valid; d.valid=false; return *this;}

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
std::string demangle(const std::type_info& type);
std::string demangle(const std::type_index& type);
std::string demangle(const char* mangled_name);

}