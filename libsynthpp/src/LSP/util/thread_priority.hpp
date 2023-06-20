#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/Id.hpp>

namespace lsp
{

// スレッドの優先度
enum class ThreadPriority
{
	Inherited, // default:do-nothing

	Lowest,
	BelowNormal,
	Normal,
	AboveNormal,
	Highest,	
};
}

namespace lsp::this_thread
{
// スレッド優先度を指定します
void set_priority(ThreadPriority p);

}