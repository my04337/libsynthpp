#include <lsp/base/logging.hpp>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace lsp;

static clock::time_point sLogStartTime = clock::now();

std::recursive_mutex Log::sMutex;
LogLevel Log::sLogLevel = LogLevel::Silent;
std::list<ILogger*> Log::sLoggers;

void Log::setLogLevel(LogLevel level)
{
	std::lock_guard<decltype(sMutex)> lock(sMutex);

	sLogLevel = level;
}
LogLevel Log::getLogLevel()
{
	std::lock_guard<decltype(sMutex)> lock(sMutex);

	return sLogLevel;
}

void Log::addLogger(ILogger* logger)
{
	std::lock_guard<std::recursive_mutex> lock(sMutex);

	auto found = std::find_if(sLoggers.begin(), sLoggers.end(),
		[&](ILogger* d){ return d == logger; });
	if(found != sLoggers.end()) return;

	sLoggers.push_back(logger);
}

void Log::removeLogger(ILogger *logger)
{
	std::lock_guard<std::recursive_mutex> lock(sMutex);

	auto found = std::find_if(sLoggers.begin(), sLoggers.end(),
		[&](ILogger* d){ return d== logger; });
	if(found == sLoggers.end()) return;

	sLoggers.erase(found);
}


#define LSP_IMPL_LOG_FUNC(level) \
    void Log::level(std::string_view text)noexcept \
    { write(LogLevel::level, [text](auto&_){_ <<text;}, nullptr);} \
    void Log::level(const Writer& writer)noexcept \
    { write(LogLevel::level, writer, nullptr);}

LSP_IMPL_LOG_FUNC(v)
LSP_IMPL_LOG_FUNC(d)
LSP_IMPL_LOG_FUNC(i)
LSP_IMPL_LOG_FUNC(w)
LSP_IMPL_LOG_FUNC(e)
[[noreturn]] void Log::f(std::string_view text, const std::stacktrace& stacks)noexcept
{ write(LogLevel::f, [text](auto& _) {_ << text; }, &stacks, true); /*到達しないはず*/ std::terminate(); }
[[noreturn]] void Log::f(const Writer& writer, const std::stacktrace& stacks)noexcept
{ write(LogLevel::f, writer, &stacks, true); /*到達しないはず*/ std::terminate();}

#undef LSP_IMPL_LOG_FUNC


void Log::write(LogLevel level, const Writer& writer, const std::stacktrace* stacks, bool isCritical)noexcept
{
	// ロック取得
	std::lock_guard<std::recursive_mutex> lock(sMutex);

	// 出力ログレベル未満なら中断
	auto currentLevel = sLogLevel;
	if(static_cast<int>(level) < static_cast<int>(currentLevel)) return;

	// 有効なロガーがない場合は中断
	bool hasLogger = false;
	for(ILogger* logger : sLoggers) {
		if(logger->isWritable(level)) {
			hasLogger = true;
			break;
		}
	}
	if(!hasLogger) return;

	// ログ時刻生成
	auto time = clock::now();

	// ログ生成
	std::ostringstream oss;
	writer(oss);
	std::unique_ptr<std::stacktrace> tmpStackTrace;
	if(level >= LogLevel::Fatal) {
		oss << std::endl;
		if(!stacks) {
			tmpStackTrace = std::make_unique<std::stacktrace>(std::stacktrace::current());
			stacks = tmpStackTrace.get();
		}
	}
	auto s = oss.str();

	// ログ出力
	for(ILogger* logger : sLoggers) {
		bool enable = logger->isWritable(level);
		bool output = enable && (!isCritical || logger->canOutputCriticalLog());
		bool flush = (output && level >= LogLevel::Info);
		if(output) logger->write(time, level, s, stacks);
		if(flush) logger->flush();
	}

	// Fatal : 強制終了
	if(level >= LogLevel::Fatal ){
		for(ILogger* logger : sLoggers) {
			if(logger->canOutputCriticalLog()) {
				logger->flush();
			}
		}
		sLoggers.clear(); // 終了処理中にさらにログ出力が呼ばれないようにする

						  // !!! Assertion Failed !!!
		std::terminate();
	}
}

void Log::flush()noexcept
{
	// ロック取得
	std::lock_guard<std::recursive_mutex> lock(sMutex);

	// ログ フラッシュ
	for(ILogger* logger : sLoggers) {
		logger->flush();
	}
}

std::ostringstream& Log::format_default(std::ostringstream& stream, clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept
{
	const std::thread::id thid = std::this_thread::get_id();

	constexpr auto SP  = " ";
	
	// 時刻
	auto timeFromStart = std::chrono::duration_cast<std::chrono::microseconds>(time - sLogStartTime).count();
	{
		int micros = static_cast<int>(timeFromStart % 1000); timeFromStart /= 1000;
		int millis = static_cast<int>(timeFromStart % 1000); timeFromStart /= 1000;
		int seconds = static_cast<int>(timeFromStart % 60); timeFromStart /= 60;
		int minutes = static_cast<int>(timeFromStart);

		stream 
			<< std::setw(2) << std::setfill('0') << minutes   << ":"
			<< std::setw(2) << std::setfill('0') << seconds   << "."
			<< std::setw(3) << std::setfill('0') << millis    << "'"
			<< std::setw(3) << std::setfill('0') << micros    << SP;
	}
	// レベル
	switch(level) {
	case LogLevel::v: stream << "V"; break;
	case LogLevel::d: stream << "D"; break;
	case LogLevel::i: stream << "I"; break;
	case LogLevel::w: stream << "W"; break;
	case LogLevel::e: stream << "E"; break;
	case LogLevel::f: stream << "F"; break;
	case LogLevel::s: stream << "S"; break;
	}
	stream << SP;


	// スレッドID
	stream << std::hex << std::setfill('0') << std::setw(4) << uint16_t(std::hash<std::thread::id>()(thid)) << std::dec << SP;
	
	// ログ
	stream << log;

	// スタックトレース
	if(stacks) stream << std::ends << *stacks;
	
	return stream;
}

// ---

StdOutLogger::StdOutLogger(bool showHeader)
	: mShowHeader(showHeader)
{

}

StdOutLogger::~StdOutLogger()
{
	flush();
}

void StdOutLogger::write(clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept
{
	std::ostringstream oss;
	if(mShowHeader) {
		Log::format_default(oss, time, level, log, stacks) << std::ends;
	} else {
		oss << log << std::ends;
		if(stacks) oss << *stacks;
	}
	std::cout << oss.str() << std::endl; // ロガーの性質上、常にflushすべき
}


void StdOutLogger::flush()noexcept
{
	std::cout.flush();
}


// ---
#ifdef _WIN32
OutputDebugStringLogger::OutputDebugStringLogger(bool showHeader)
	: mShowHeader(showHeader)
{

}

OutputDebugStringLogger::~OutputDebugStringLogger()
{
	flush();
}

void OutputDebugStringLogger::write(clock::time_point time, LogLevel level, std::string_view log, const std::stacktrace* stacks)noexcept
{
	std::ostringstream oss;
	if(mShowHeader) {
		Log::format_default(oss, time, level, log, stacks) << std::ends;
	} else {
		oss << log << std::ends;
		if(stacks) oss <<  *stacks;
	}
	OutputDebugStringA(oss.str().c_str());
	OutputDebugStringA("\r\n");
}


void OutputDebugStringLogger::flush()noexcept
{
	// do-nothing
}

#endif