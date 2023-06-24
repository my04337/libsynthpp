#include <lsp/util/thread_priority.hpp>
#include <lsp/base/logging.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

using namespace lsp;

void lsp::this_thread::set_priority(ThreadPriority p)
{
#if defined(WIN32)
	int nPriority = THREAD_PRIORITY_NORMAL;
	switch (p) {
	case ThreadPriority::Lowest:
		nPriority = THREAD_PRIORITY_LOWEST;
		break;
	case ThreadPriority::BelowNormal:
		nPriority = THREAD_PRIORITY_BELOW_NORMAL;
		break;
	case ThreadPriority::Normal:
		nPriority = THREAD_PRIORITY_NORMAL;
		break;
	case ThreadPriority::AboveNormal:
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
		break;
	case ThreadPriority::Highest:
		nPriority = THREAD_PRIORITY_HIGHEST;
		break;
	}
	::SetThreadPriority(::GetCurrentThread(), nPriority);
#else
	// このプラットフォームでは未対応
#endif
}