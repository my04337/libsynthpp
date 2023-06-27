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
	
	static void v(std::string_view text)noexcept;
	static void v(const Writer& writer)noexcept;

	static void d(std::string_view text)noexcept;
	static void d(const Writer& writer)noexcept;

	static void i(std::string_view text)noexcept;
	static void i(const Writer& writer)noexcept;

	static void w(std::string_view text)noexcept;
	static void w(const Writer& writer)noexcept;

	static void e(std::string_view text)noexcept;
	static void e(const Writer& writer)noexcept;

	[[noreturn]]
	static void f(std::string_view text, const std::stacktrace& stacks = std::stacktrace::current())noexcept;
	[[noreturn]]
	static void f(const Writer& writer, const std::stacktrace& stacks = std::stacktrace::current())noexcept;


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

	virtual bool isWritable(LogLevel level)const noexcept override { return true; }
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

		virtual bool isWritable(LogLevel level)const noexcept override { return true; }
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
		lsp::Log::f([location](auto& _) {_ << location.file_name() << ":" << location.line() << " - illegal argument."; });
	}
}
inline static void require(bool succeeded, std::string_view description, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		lsp::Log::f([description, location](auto& o) {o << location.file_name() << ":" << location.line() << " - " << description; });
	}
}
template<typename D>
	requires std::invocable<D, std::ostringstream&>
inline static void require(bool succeeded, D&& description, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		lsp::Log::f([description = std::forward<D>(description), location](auto& o) {o << location.file_name() << ":" << location.line() << " - "; description(o); });
	}
}

// check : 関数の内部での状態チェック向け
inline static void check(bool succeeded, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		lsp::Log::f([location](auto& _) {_ << location.file_name() << ":" << location.line() << " - illegal state."; });
	}
}
inline static void check(bool succeeded, std::string_view description, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		lsp::Log::f([description, location](auto& o) {o << location.file_name() << ":" << location.line() << description; });
	}
}
template<typename D>
	requires std::invocable<D, std::ostringstream&>
inline static void check(bool succeeded, D&& description, std::source_location location = std::source_location::current()) {
	if(!succeeded) [[unlikely]] {
		lsp::Log::f([description = std::forward<D>(description), location](auto& o) {o << location.file_name() << ":" << location.line() << " - "; description(o); });
	}
}

// unreachable : 到達不可能な箇所でのチェック向け
[[noreturn]]
inline static void unreachable(std::source_location location = std::source_location::current()) {
	lsp::Log::f([location](auto& _) {_ << location.file_name() << ":" << location.line() << " - illegal state."; });
}
[[noreturn]]
inline static void unreachable(std::string_view description, std::source_location location = std::source_location::current()) {
	lsp::Log::f([description, location](auto& o) {o << location.file_name() << ":" << location.line() << description; });
}
template<typename D>
	requires std::invocable<D, std::ostringstream&>
[[noreturn]]
inline static void unreachable(D&& description, std::source_location location = std::source_location::current()) {
	lsp::Log::f([description = std::forward<D>(description), location](auto& o) {o << location.file_name() << ":" << location.line() << " - "; description(o); });
}
}