﻿/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>

namespace lsp
{

// スレッドの優先度
enum class ThreadPriority
{
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