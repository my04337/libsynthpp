#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>
#include <LSP/Base/StackTrace.hpp>


// ログ用ユーティリティマクロ
#define LOGF(...) [&](std::ostringstream& _)->void { _ << __VA_ARGS__; }

// assertマクロ(記述しやすいようあえて小文字化)
#define lsp_assert(expr) \
    if(!(expr)) { \
        LSP::Log::f(LOGF(LSP::file_macro_to_filename(__FILE__) << ":" << __LINE__ << " - assert" << "(" << DELAY_MACRO(#expr) << ") failed.")); \
    }
#define lsp_assert_desc(expr, ...) \
    if(!(expr)) { \
        LSP::Log::f(LOGF(LSP::file_macro_to_filename(__FILE__) << ":" << __LINE__ << " - assert" << "(" << DELAY_MACRO(#expr) << ") failed " << __VA_ARGS__ ; _ << "].")); \
    }

// 開発時用デバッグログ(コミット前に除去すること)
#define lsp_debug_log(expr) \
	LSP::Log::w(LOGF(LSP::file_macro_to_filename(__FILE__) << ":" << __LINE__  << " - " << expr));

#define lsp_dump_stacktrace() \
	lsp_debug_log(""; LSP::Log::printStackTrace(_, LSP::Log::getStackTrace()));


namespace LSP
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
	// ログで使用するクロック
	using clock = std::chrono::steady_clock;
	// ログで使用するタイムポイント
	using time_point = clock::time_point;
	// ログで使用する期間
	using duration = clock::duration;
	// ログ出力式
	using Writer = std::function<void(std::ostringstream& o)>;
	// スタックトレース
	using StackTraceElement = CppCallStack::StackTraceElement;
	using StackTrace = CppCallStack::StackTrace;

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
	static void f(std::string_view text, const StackTrace* stacks=nullptr)noexcept;
	[[noreturn]]
	static void f(const Writer& writer, const StackTrace* stacks=nullptr)noexcept;

	// ログ フラッシュ [例外送出禁止]
	static void flush()noexcept;

	// 実際の書き込み処理 [例外送出禁止]
	static void write(LogLevel level, const Writer& writer, const StackTrace* stacks=nullptr, bool isCritical=false)noexcept;

	// 標準ログフォーマットで整形 [例外送出禁止]
	static std::ostringstream& format_default(std::ostringstream& out, Log::time_point time, LogLevel level, std::string_view log, const Log::StackTrace* stacks)noexcept;

	// スタックトレースの取得 [常用禁止 : 使用するとデバッグ情報が大量に読み込まれメインメモリを圧迫するため]
	static StackTrace getStackTrace(size_t skipFrames=0)noexcept;
	
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
	virtual void write(Log::time_point time, LogLevel level, std::string_view log, const Log::StackTrace* stacks)noexcept = 0;

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
	StdOutLogger(bool showHeader=true);
	virtual ~StdOutLogger();

	// --- ILogger ---
	// ログ書き込み (stacks!=null:スタックトレース出力)
	virtual void write(Log::time_point time, LogLevel level, std::string_view log, const Log::StackTrace* stacks)noexcept override;
	virtual void flush()noexcept override;

	virtual bool isWritable(LogLevel level)const noexcept override { return true; }
	virtual bool canOutputCriticalLog()const noexcept override { return true; }
	
private:
	bool mShowHeader;
};

#ifdef WIN32
namespace Win32
{
/// ロガー : OutputDebugString用
class OutputDebugStringLogger final
	: public ILogger
{
public:
	OutputDebugStringLogger(bool showHeader=true);
	virtual ~OutputDebugStringLogger();

	// --- ILogger ---
	// ログ書き込み (stacks!=null:スタックトレース出力)
	virtual void write(Log::time_point time, LogLevel level, std::string_view log, const Log::StackTrace* stacks)noexcept override;
	virtual void flush()noexcept override;

	virtual bool isWritable(LogLevel level)const noexcept override { return true; }
	virtual bool canOutputCriticalLog()const noexcept override { return true; }

private:
	bool mShowHeader;
};
}
#endif

}