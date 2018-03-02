#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>
#include <LSP/Debugging/StackTrace.hpp>


// ログ用ユーティリティマクロ
#define LOGF(expr) [&](ostringstream_t& _)->void { _ << expr; }
#define LSP_LOGUTIL_FILENAME \
	LSP::strings::file_macro_to_filename(__FILE__)

// assertマクロ(記述しやすいようあえて小文字化)
#define lsp_assert(expr) \
    if(!(expr)) { \
        LSP::Log::f(LOGF(LSP_LOGUTIL_FILENAME << L":" << __LINE__ << L" - assert" << L"(" << DELAY_MACRO(u###expr) << L") failed."))); \
    }
#define lsp_assert_desc(expr, ...) \
    if(!(expr)) { \
        LSP::Log::f(LOGF(LSP_LOGUTIL_FILENAME << L":" << __LINE__ << L" - assert" << L"(" << DELAY_MACRO(u###expr) << L") failed [" << __VA_ARGS__ ; _ << L"]."))); \
    }

// 開発時用デバッグログ(コミット前に除去すること)
#define lsp_debug_log(expr) \
	LSP::Log::w(LOGF(LSP_LOGUTIL_FILENAME << L":" << __LINE__  << L" - " << expr));

#define lsp_dump_stacktrace() \
	lsp_debug_log(L""; LSP::Log::printStackTrace(_, LSP::Log::getStackTrace()));


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
	using Writer = std::function<void(ostringstream_t& o)>;
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
	static void v(const string_t& text, bool isCritical=false)noexcept;
	static void v(const Writer& writer, bool isCritical=false)noexcept;

	static void d(const string_t& text, bool isCritical=false)noexcept;
	static void d(const Writer& writer, bool isCritical=false)noexcept;

	static void i(const string_t& text, bool isCritical=false)noexcept;
	static void i(const Writer& writer, bool isCritical=false)noexcept;

	static void w(const string_t& text, bool isCritical=false)noexcept;
	static void w(const Writer& writer, bool isCritical=false)noexcept;

	static void e(const string_t& text, bool isCritical=false)noexcept;
	static void e(const Writer& writer, bool isCritical=false)noexcept;

	[[noreturn]]
	static void f(const string_t& text, const StackTrace* stacks=nullptr)noexcept;
	[[noreturn]]
	static void f(const Writer& writer, const StackTrace* stacks=nullptr)noexcept;

	// ログ フラッシュ [例外送出禁止]
	static void flush()noexcept;

	// 実際の書き込み処理 [例外送出禁止]
	static void write(LogLevel level, const Writer& writer, const StackTrace* stacks=nullptr, bool isCritical=false)noexcept;

	// 標準ログフォーマットで整形 [例外送出禁止]
	static ostringstream_t& format_default(ostringstream_t& out, Log::time_point time, LogLevel level, const string_t& log, const Log::StackTrace* stacks)noexcept;

	// スタックトレースの取得/表示 [常用禁止 : 使用するとデバッグ情報が大量に読み込まれメインメモリを圧迫するため]
	static StackTrace getStackTrace(size_t skipFrames=0)noexcept;
	static void printStackTrace(ostringstream_t& stream, const StackTrace& st, size_t max_stack_num = std::numeric_limits<size_t>::max())noexcept;
	
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
	virtual void write(Log::time_point time, LogLevel level, const string_t& log, const Log::StackTrace* stacks)noexcept = 0;

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
	virtual void write(Log::time_point time, LogLevel level, const string_t& log, const Log::StackTrace* stacks)noexcept override;
	virtual void flush()noexcept override;

	virtual bool isWritable(LogLevel level)const noexcept override { return true; }
	virtual bool canOutputCriticalLog()const noexcept override { return true; }

	// ---
	bool valid()const noexcept;

private:
	bool mShowHeader;
};

}