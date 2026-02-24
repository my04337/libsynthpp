// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

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
#include <ranges>
#include <semaphore>
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

// --- 基本的なユーティリティ (MIT-0) ---
#include <lsp/core/basic_utility.hpp>

// --- 基本的な型 ---
namespace lsp
{
// クロック類
using clock = std::chrono::steady_clock;

} // namespace lsp
