#include <LSP/Stream/Message.hpp>

using namespace LSP;
using namespace LSP::Stream;

Message::Message()
	: mPosition(0)
	, mOp(Nop)
{
}
Message::Message(position_t pos, OpType op, std::any&& param)
	: mPosition(pos)
	, mOp(op)
	, mParam(std::move(param))
{
}
bool Message::operator<(const Message& rhs)const noexcept
{
	return mPosition < rhs.mPosition;
}
bool Message::operator>(const Message& rhs)const noexcept
{
	return mPosition > rhs.mPosition;
}

Message::OpType Message::op()const noexcept 
{
	return mOp; 
}

position_t Message::position()const noexcept 
{
	return mPosition; 
}

const std::any& Message::param()const noexcept 
{ 
	return mParam;
}
std::any& Message::param()noexcept 
{
	return mParam;
}