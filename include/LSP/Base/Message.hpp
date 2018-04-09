#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP
{

// 制御メッセージ
class Message final
	: non_copy
{
public:
	// メッセージ種別
	using OpType = uint32_t;
	static constexpr OpType Nop = 0x00000000; // [reserved] 何もしない制御メッセージ

public:
	Message();
	Message(int64_t pos, OpType op, std::any&& param={});

	Message(Message&&)noexcept = default;
	Message& operator=(Message&&) = default;

	bool operator<(const Message& rhs)const noexcept;
	bool operator>(const Message& rhs)const noexcept;
	
	// 適用時刻
	int64_t position()const noexcept;

	// 制御命令種別 (受信先により解釈が異なります)
	OpType op()const noexcept;

	// 制御パラメータ(受信先と制御命令種別により解釈が異なります)
	const std::any& Message::param()const noexcept;
	      std::any& Message::param()noexcept;
    
private:
	int64_t mPosition;
	OpType mOp;
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
	std::optional<Message> pop();
	std::optional<Message> pop(int64_t until = std::numeric_limits<int64_t>::max());

private:
	mutable std::mutex mQueueMutex;
	std::deque<Message> mQueue;
};

}