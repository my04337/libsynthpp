#pragma once

#include <LSP/minimal.hpp>

namespace LSP::Test
{

class ITest : non_copy_move
{
public:
	virtual ~ITest(){}

	// テストカテゴリ
	virtual std::string_view category() = 0;

	// テストコマンド
	virtual std::string_view command() = 0;

	// テスト詳細
	virtual std::string_view description() = 0;

	//テスト実行
	virtual void exec() = 0;

private:
};

}