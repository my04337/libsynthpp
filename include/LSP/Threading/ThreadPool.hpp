#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Threading/Task.hpp>

namespace LSP
{
class Dispatcher;

// スレッドプール : 割り付けられたタスクを処理するスレッドを管理する
class ThreadPool final
	: non_copy_move
{
public:
	ThreadPool(Dispatcher& dispatcher, size_t thread_num = 0);
	~ThreadPool();
	
protected:
	void threadMain();


private:
	Dispatcher& mDispatcher;
	std::atomic_bool mAborted;
	std::unordered_map<std::thread::id, std::unique_ptr<std::thread>> mThreads;
};


}