#include "test_base.hpp"

#include <LSP/Base/Message.hpp>

using namespace LSP;


void Test::BaseTest::exec()
{
	// Message
	Log::d("Testing : Base");
	{
		auto expect_empty = [](const auto& v)->bool{ return !v; };
		auto expect_op = [](const auto& v, uint32_t op)->bool{ return v && v->op() == op; };

		MessageQueue mq;
		lsp_assert(expect_empty(mq.pop()));

		mq.push(Message(0, 1));
		lsp_assert(expect_op(mq.pop(), 1));
		lsp_assert(expect_empty(mq.pop()));

		mq.push(Message(100, 10));
		mq.push(Message(100, 11));
		mq.push(Message(200, 12));
		mq.push(Message(150, 13));
		mq.push(Message(100, 14));
		lsp_assert(expect_empty(mq.pop(0)));
		lsp_assert(expect_empty(mq.pop(100)));
		lsp_assert(expect_op(mq.pop(101), 10));
		lsp_assert(expect_empty(mq.pop(100)));
		lsp_assert(expect_op(mq.pop(101), 11));
		lsp_assert(expect_op(mq.pop(101), 14));
		lsp_assert(expect_empty(mq.pop(101)));
		lsp_assert(expect_op(mq.pop(151), 13));
		lsp_assert(expect_op(mq.pop(201), 12));
		lsp_assert(expect_empty(mq.pop()));
	}

	Log::d("Testing : Base - End");
}