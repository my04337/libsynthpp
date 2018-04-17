#include <LSP/Threading/Thread.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

using namespace LSP;
using namespace LSP::Threading;

void LSP::Threading::setThreadPriority(std::thread& th, Priority p)
{
#if defined(WIN32)
	int nPriority = THREAD_PRIORITY_NORMAL;
	switch (p) {
	case Priority::Lowest:	
		nPriority = THREAD_PRIORITY_LOWEST;
		break;
	case Priority::BelowNormal:
		nPriority = THREAD_PRIORITY_BELOW_NORMAL;
		break;
	case Priority::Normal:
		nPriority = THREAD_PRIORITY_NORMAL;
		break;
	case Priority::AboveNormal:
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
		break;
	case Priority::Highest:
		nPriority = THREAD_PRIORITY_HIGHEST;
		break;
	}
	auto hThread = reinterpret_cast<HANDLE>(th.native_handle());
	::SetThreadPriority(hThread, nPriority);
#else
	// このプラットフォームでは未対応
#endif
}