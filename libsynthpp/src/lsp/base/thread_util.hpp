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

// スレッド優先度を指定します
void setThreadPriority(std::thread& th, ThreadPriority p);

}