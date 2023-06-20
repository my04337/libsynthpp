#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/Id.hpp>

namespace LSP::Threading
{

// スレッドの優先度
enum class Priority
{
	Inherited, // default:do-nothing

	Lowest,
	BelowNormal,
	Normal,
	AboveNormal,
	Highest,	
};

// スレッド優先度を指定します
void setThreadPriority(std::thread& th, Priority p);

}