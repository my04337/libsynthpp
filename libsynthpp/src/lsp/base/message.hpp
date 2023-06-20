#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/id.hpp>

namespace lsp
{
	 
// 制御メッセージ
class Message final
	: non_copy
{
public:
	Message();
	Message(int64_t pos, uint32_t op, std::any&& param={});

	Message(Message&&)noexcept = default;
	Message& operator=(Message&&) = default;

	bool operator<(const Message& rhs)const noexcept;
	bool operator>(const Message& rhs)const noexcept;
	
	// 適用時刻
	int64_t position()const noexcept;

	// 制御命令種別 (受信先により解釈が異なります)
	uint32_t op()const noexcept;

	// 制御パラメータ(受信先と制御命令種別により解釈が異なります)
	const std::any& param()const noexcept;
	      std::any& param()noexcept;
    
private:
	int64_t mPosition;
	uint32_t mOp;
	std::any mParam;
};


// 制御メッセージ キュー
class MessageQueue final
	: non_copy
{
public:

	// メッセージをキューイングします
	void push(Message&& msg);

	// キューイングされたメッセージを取得します (ただし時刻がuntilより前のものに限る)
	std::optional<Message> pop(int64_t until = std::numeric_limits<int64_t>::max());

private:
	mutable std::mutex mQueueMutex;
	std::deque<Message> mQueue;
};

}