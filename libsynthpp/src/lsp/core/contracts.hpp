// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/logging.hpp>

// ===== 契約マクロ =====
// C++26 Contracts (P2900R14) 導入時の移行対応表:
//   lsp_require(cond)  →  pre(cond)            : 事前条件 (引数チェック)
//   lsp_ensure(cond)   →  post(cond)           : 事後条件 (戻り値チェック)
//   lsp_check(cond)    →  contract_assert(cond) : 表明 (内部状態チェック)
//
// いずれもプログラミングエラー(バグ)の検出専用。
// 実行時のリカバリ可能なエラーには使用しないこと。

// require : 事前条件 - 関数の引数が満たすべき条件
#define lsp_require(...) \
	if(!(__VA_ARGS__)) [[unlikely]] { \
		lsp::Log::f(std::stacktrace::current(), "precondition violation: '{}'.", #__VA_ARGS__ ); \
		std::unreachable(); \
	}

// ensure : 事後条件 - 関数の戻り値・出力が満たすべき条件
#define lsp_ensure(...) \
	if(!(__VA_ARGS__)) [[unlikely]] { \
		lsp::Log::f(std::stacktrace::current(), "postcondition violation: '{}'.", #__VA_ARGS__ ); \
		std::unreachable(); \
	}

// check : 表明 - 関数内部の不変条件
#define lsp_check(...) \
	if(!(__VA_ARGS__)) [[unlikely]] { \
		lsp::Log::f(std::stacktrace::current(), "assertion violation: '{}'.", #__VA_ARGS__ ); \
		std::unreachable(); \
	}

// ===== リアルタイムスレッド用エラーリカバリ =====
//
// リアルタイムスレッド(オーディオ演奏スレッド等)上では、例外送出やabortによる
// 中断を避け、可能な限り処理を継続する必要がある。
// このマクロはエラーをログに記録したうえで、指定されたリカバリ処理を実行する。
//
// 使用例:
//   if(index >= size) lsp_rt_fail(return {}, "buffer overrun: index={}", index);
//   if(!std::isfinite(v)) lsp_rt_fail(continue, "NaN/Inf detected on ch={}", ch);
//
#define lsp_rt_fail(recovery_action, ...) \
	do { \
		lsp::Log::e(__VA_ARGS__); \
		recovery_action; \
	} while(false)
