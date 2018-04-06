#pragma once

#include <LSP/Stream/StreamBase.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP::Stream
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
	Message(position_t pos, OpType op, std::any&& param={});

	Message(Message&&)noexcept = default;
	Message& operator=(Message&&) = default;

	bool operator<(const Message& rhs)const noexcept;
	bool operator>(const Message& rhs)const noexcept;
	
	// 適用時刻
	position_t position()const noexcept;

	// 制御命令種別 (受信先により解釈が異なります)
	OpType op()const noexcept;

	// 制御パラメータ(受信先と制御命令種別により解釈が異なります)
	const std::any& Message::param()const noexcept;
	      std::any& Message::param()noexcept;
    
private:
	position_t mPosition;
	OpType mOp;
	std::any mParam;
};

}