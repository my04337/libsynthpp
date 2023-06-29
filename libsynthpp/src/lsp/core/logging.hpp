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
	
	static void v(std::string_view text)noexcept 
	{ write(LogLevel::v, [text](std::ostringstream& o) {o << text; }); }
	template<typename... Args>
	static void v(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::v, [&](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	static void d(std::string_view text)noexcept
	{ write(LogLevel::d, [text](std::ostringstream& o) {o << text; }); }
	template<typename... Args>
	static void d(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::d, [&](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	static void i(std::string_view text)noexcept 
	{ write(LogLevel::i, [text](std::ostringstream& o) {o << text; }); }
	template<typename... Args>
	static void i(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::i, [&](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	static void w(std::string_view text)noexcept 
	{ write(LogLevel::w, [text](std::ostringstream& o) {o << text; }); }
	template<typename... Args>
	static void w(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::w, [&](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	static void e(std::string_view text)noexcept
	{ write(LogLevel::e, [text](std::ostringstream& o) {o << text; }); }
	template<typename... Args>
	static void e(std::format_string<Args...> s, Args&&... args)noexcept 
	{ write(LogLevel::e, [&](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }); }

	[[noreturn]]
	static void f(std::string_view text, const std::stacktrace& stacks = std::stacktrace::current())noexcept
	{ write(LogLevel::f, [text](std::ostringstream& o) {o << text; }, &stacks, true); std::unreachable(); }
	template<typename... Args>
	[[noreturn]]
	static void f(const std::stacktrace& stacks, std::format_string<Args...> s, Args&&... args)noexcept
	{ write(LogLevel::f, [&](std::ostringstream& o) {o << std::format(s, std::forward<Args>(args)...); }, &stacks, true); std::unreachable(); }


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
namespace lsp::inline assertion
{
// require : 引数チェック向け
inline static void require(bool succeeded, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		auto stacks = std::stacktrace::current(1);
		lsp::Log::write(LogLevel::f, [&](std::ostringstream& o) { o << std::format("{}:{} - illegal argument.", location.file_name(), location.line()); }, &stacks, true);
		std::unreachable();
	}
}
inline static void require(bool succeeded, std::string_view description, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		auto stacks = std::stacktrace::current(1);
		lsp::Log::write(LogLevel::f, [&](std::ostringstream& o) { o << std::format("{}:{} - {}", location.file_name(), location.line(), description); }, &stacks, true);
		std::unreachable();
	}
}

// check : 関数の内部での状態チェック向け
inline static void check(bool succeeded, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		auto stacks = std::stacktrace::current(1);
		lsp::Log::write(LogLevel::f, [&](std::ostringstream& o) { o << std::format("{}:{} - illegal state.", location.file_name(), location.line()); }, &stacks, true);
		std::unreachable();
	}
}
inline static void check(bool succeeded, std::string_view description, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		auto stacks = std::stacktrace::current(1);
		lsp::Log::write(LogLevel::f, [&](std::ostringstream& o) { o << std::format("{}:{} - {}", location.file_name(), location.line(), description); }, &stacks, true);
		std::unreachable();
	}
}
}