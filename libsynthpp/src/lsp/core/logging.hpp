// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/base.hpp>
#include <lsp/core/id.hpp>

namespace lsp
{
class ILogger;

// ログレベル
enum class LogLevel
{
	// 詳細レベル - 普段は見る必要のない細かすぎる内容などに使用
	Verbose = 1,
	v = Verbose,

	// デバッグレベル - デバッグには必要だが、全体の流れを追うには不要な内容に使用
	Debug = 2,
	d = Debug,

	// 情報レベル - 全体の流れを追うのに便利な内容に使用
	Info = 3,
	i = Info,

	// 警告レベル - 致命的ではないが、注視すべき挙動があった場合に使用
	Warning = 4,
	w = Warning,

	// エラーレベル - 致命的ではないが、エラーとして扱う事象が発生した場合に使用
	Error = 5,
	e = Error,

	// 致命的レベル - プログラムの実行に重大な支障があるような事象が発生した場合に使用
	Fatal = 6,
	f = Fatal,

	// サイレントレベル - 何も出力しない
	Silent = 7,
	s = Silent,
};

// ロガー(static)
class Log final
	: non_copy_move
{
public:
	// ログ出力式
	using Writer = std::function<void(std::ostringstream& o)>;

public:
	// グローバル ログレベル
	static void setLogLevel(LogLevel level);
	static LogLevel getLogLevel();
	
	// ログ出力先追加
	static void addLogger(ILogger* logger);

	// ログ出力先除去
	static void removeLogger(ILogger* logger);

	// ログ書き込み [例外送出禁止]	
	template<typename... Args>
	static void v(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::v, [&, s = std::move(s)](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	template<typename... Args>
	static void d(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::d, [&, s = std::move(s)](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	template<typename... Args>
	static void i(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::i, [&, s = std::move(s)](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	template<typename... Args>
	static void w(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::w, [&, s = std::move(s)](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	template<typename... Args>
	static void e(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::e, [&, s = std::move(s)](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	template<typename... Args>
	[[noreturn]]
	static void f(const std::stacktrace& stacks, std::format_string<Args...> s, Args&&... args)noexcept
	{ write(LogLevel::f, [&, s = std::move(s)](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }, &stacks, true); std::unreachable(); }


	// ログ フラッシュ [例外送出禁止]
	static void flush()noexcept;

	// 実際の書き込み処理 [例外送出禁止]
	static void write(LogLevel level, const Writer& writer, const std::stacktrace* stacks=nullptr, bool isCritical=false)noexcept;


	// 標準ログフォーマットで整形 [例外送出禁止]
	static std::ostringstream& format_default(std::ostringstream& out, clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept;
	
private:
	// 書き込みロック
	static std::recursive_mutex sMutex;

	// ログレベル
	static LogLevel sLogLevel;

	// 書き込み先ロガー
	static std::list<ILogger*> sLoggers;
};
// ログ出力先インタフェース
class ILogger
	: non_copy_move
{
public:
	using LogFilter = std::function<bool(LogLevel level)noexcept>;

	virtual ~ILogger() {}

	// ログ出力が可能なログレベルかを判定
	virtual bool isWritable(LogLevel level)const noexcept = 0;

	// ログ書き込み (stacks!=null:スタックトレース出力)
	virtual void write(clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept = 0;

	// 書き込み済みログのフラッシュ
	virtual void flush()noexcept = 0;

	// 特殊なコールバックからのログ出力が可能か否か(メインスレッドと同期を取るようなロガーは出力不可)
	virtual bool canOutputCriticalLog()const noexcept = 0;
};

/// ロガー : 標準出力
class StdOutLogger final
	: public ILogger
{
public:
	StdOutLogger(bool showHeader = true);
	virtual ~StdOutLogger();

	// --- ILogger ---
	// ログ書き込み (stacks!=null:スタックトレース出力)
	virtual void write(clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept override;
	virtual void flush()noexcept override;

	virtual bool isWritable([[maybe_unused]] LogLevel level)const noexcept override { return true; }
	virtual bool canOutputCriticalLog()const noexcept override { return true; }

private:
	bool mShowHeader;
};
}
#ifdef WIN32
namespace lsp
{
	/// ロガー : OutputDebugString用
	class OutputDebugStringLogger final
		: public ILogger
	{
	public:
		OutputDebugStringLogger(bool showHeader = true);
		virtual ~OutputDebugStringLogger();

		// --- ILogger ---
		// ログ書き込み (stacks!=null:スタックトレース出力)
		virtual void write(clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept override;
		virtual void flush()noexcept override;

		virtual bool isWritable([[maybe_unused]] LogLevel level)const noexcept override { return true; }
		virtual bool canOutputCriticalLog()const noexcept override { return true; }

	private:
		bool mShowHeader;
	};
}
#endif

// アサーション機構
// MEMO 契約プログラミングのアノテーション形式で記述したいが、実装まではマクロとする。 C++モジュールは当分考慮しない。

// require : 引数チェック用
#define lsp_require(...) \
	if(!(__VA_ARGS__)) [[unlikely]] { \
		lsp::Log::f(std::stacktrace::current(), "illegal argument. expected : '{}'.", #__VA_ARGS__ ); \
		std::unreachable(); \
	}

// check : 関数の内部での状態チェック用
#define lsp_check(...) \
	if(!(__VA_ARGS__)) [[unlikely]] { \
		lsp::Log::f(std::stacktrace::current(), "illegal state. expected : '{}'.", #__VA_ARGS__ ); \
		std::unreachable(); \
	}