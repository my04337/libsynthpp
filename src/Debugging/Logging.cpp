#include <LSP/Debugging/Logging.hpp>
#include <iostream>

using namespace LSP;

static Log::time_point sLogStartTime = Log::clock::now();

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

Log::StackTrace Log::getStackTrace(size_t skipFrames)noexcept
{
	return CppCallStack::getStackTrace(skipFrames);
}
void Log::printStackTrace(ostringstream_t& stream, const StackTrace& st, size_t max_stack_num)noexcept
{
	CppCallStack::printStackTrace(stream, st, max_stack_num);
}


#define LSP_IMPL_LOG_FUNC(level) \
    void Log::level(const string_t& text, bool isCritical)noexcept \
    { write(LogLevel::level, LOGF(text), nullptr, isCritical);} \
    void Log::level(const Writer& writer, bool isCritical)noexcept \
    { write(LogLevel::level, writer, nullptr, isCritical);}

LSP_IMPL_LOG_FUNC(v)
LSP_IMPL_LOG_FUNC(d)
LSP_IMPL_LOG_FUNC(i)
LSP_IMPL_LOG_FUNC(w)
LSP_IMPL_LOG_FUNC(e)
[[noreturn]] void Log::f(const string_t& text, const StackTrace* stacks)noexcept
{ write(LogLevel::f, LOGF(text), stacks, true); /*到達しないはず*/ std::terminate(); }
[[noreturn]] void Log::f(const Writer& writer, const StackTrace* stacks)noexcept
{ write(LogLevel::f, writer, stacks, true); /*到達しないはず*/ std::terminate();}

#undef LSP_IMPL_LOG_FUNC


void Log::write(LogLevel level, const Writer& writer, const StackTrace* stacks, bool isCritical)noexcept
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
	auto time = Log::clock::now();

	// ログ生成
	ostringstream_t oss;
	writer(oss);
	std::unique_ptr<StackTrace> tmpStackTrace;
	if(level >= LogLevel::Fatal) {
		oss << std::endl;
		if(!stacks) {
			tmpStackTrace = std::make_unique<StackTrace>(getStackTrace());
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

ostringstream_t& Log::format_default(ostringstream_t& stream, Log::time_point time, LogLevel level, const string_t& log, const Log::StackTrace* stacks)noexcept
{
	const std::thread::id thid = std::this_thread::get_id();

	constexpr auto SP  = L" ";
	
	// 時刻
	auto timeFromStart = std::chrono::duration_cast<std::chrono::microseconds>(time - sLogStartTime).count();
	{
		int micros = static_cast<int>(timeFromStart % 1000); timeFromStart /= 1000;
		int millis = static_cast<int>(timeFromStart % 1000); timeFromStart /= 1000;
		int seconds = static_cast<int>(timeFromStart % 60); timeFromStart /= 60;
		int minutes = static_cast<int>(timeFromStart);

		stream 
			<< std::setw(2) << std::setfill(L'0') << minutes   << L":"
			<< std::setw(2) << std::setfill(L'0') << seconds   << L"."
			<< std::setw(3) << std::setfill(L'0') << millis    << L"'"
			<< std::setw(3) << std::setfill(L'0') << micros    << SP;
	}
	// レベル
	switch(level) {
	case LogLevel::v: stream << L"V"; break;
	case LogLevel::d: stream << L"D"; break;
	case LogLevel::i: stream << L"I"; break;
	case LogLevel::w: stream << L"W"; break;
	case LogLevel::e: stream << L"E"; break;
	case LogLevel::f: stream << L"F"; break;
	case LogLevel::s: stream << L"S"; break;
	}
	stream << SP;


	// スレッドID
	stream << std::hex << std::setfill(L'0') << std::setw(4) << uint16_t(std::hash<std::thread::id>()(thid)) << std::dec << SP;
	
	// ログ
	stream << log;

	// スタックトレース
	if(
		stacks
#ifdef Q_OS_WIN
		&& !IsDebuggerPresent() // デバッガ(CDB)がアタッチされていないときのみ有効 (低速なため)
#endif
		)
	{
		printStackTrace(stream, *stacks);
	}
	
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

void StdOutLogger::write(Log::time_point time, LogLevel level, const string_t& log, const Log::StackTrace* stacks)noexcept
{
	ostringstream_t oss;
	if(mShowHeader) {
		Log::format_default(oss, time, level, log, stacks) << std::ends;
	} else {
		oss << log << std::ends;
		if(
			stacks
#ifdef Q_OS_WIN
			&& !IsDebuggerPresent() // デバッガ(CDB)がアタッチされていないときのみ有効 (低速なため)
#endif
			)
		{
			Log::printStackTrace(oss, *stacks);
		}
	}
	std::wcout << oss.str() << std::endl; // ロガーの性質上、常にflushすべき
}


void StdOutLogger::flush()noexcept
{
	// do-nothing
}

bool StdOutLogger::valid()const noexcept
{
	return true;
}
