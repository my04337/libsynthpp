export module lsp.threading;

import lsp.core;

namespace lsp
{

// スレッドの優先度
export enum class ThreadPriority
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
	export void set_priority(ThreadPriority p);

}