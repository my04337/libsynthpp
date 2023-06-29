/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

// #  基本的なヘッダおよびマクロ ---
// プラットフォーム非依存
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <format>
#include <functional>
#include <future>
#include <latch>
#include <limits>
#include <list>
#include <memory_resource>
#include <memory>
#include <mutex>
#include <numbers>
#include <optional>
#include <source_location>
#include <stacktrace>
#include <string_view>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// プラットフォーム非依存 : JUCE ※ICE License Only
#ifndef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
	#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
#endif
#ifdef WIN32
	#pragma pack(push)
	#pragma warning(disable:4996)
#endif
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#ifdef WIN32
	#pragma pack(pop)
#endif

// プラットフォーム依存 : Win32
#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN 
	#define STRICT 
	#define NOMINMAX 
	#include <sdkddkver.h>
	#include <Windows.h>
	#include <atlbase.h>
	#include <atlcom.h>

#endif

// --- 基本的な型 ---
namespace lsp
{
// クロック類
using clock = std::chrono::steady_clock;

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

// 添字演算子可能である事を表すコンセプト
template<typename Tcontainer, typename Telement>
concept subscript_operator_available = requires(Tcontainer a, size_t index) {
	{a[index]} -> std::convertible_to<Telement>;
};
}