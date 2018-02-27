#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/String.hpp>

namespace LSP
{

// スタックトレース 取得or生成
struct CppCallStack
{
	// スタックトレース
	using StackTraceElement = void*;
	using StackTrace = std::vector<StackTraceElement>;

	// スタックトレースの取得
	static StackTrace getStackTrace(size_t skipFrames=0)noexcept;
	static StackTrace getStackTraceSEH(void* hThread, void* pContextRecord)noexcept;

	// (大まかな)スタックサイズの取得
	static size_t getStackSize()noexcept;
	static void setStackBottom()noexcept;

	// スタックトレースの出力 [例外送出禁止]
	static void printStackTrace(ostream_t& stream, const StackTrace& st, size_t max_stack_num = std::numeric_limits<size_t>::max())noexcept;
};


}