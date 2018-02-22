#pragma once

#include <LSP/Base/base.hpp>

namespace LSP
{

/// 信号型
class ISignal 
	: non_copy_move
{
public:
	virtual ~ISignal() {}

	// データへのポインタを取得します
	virtual float_t* data() = 0;

	// データ数を取得します。
	virtual size_t size()const  = 0;
};

/// 信号ソース
class ISignalSource 
	: non_copy_move
{
public:
	virtual ~ISignalSource() {}

	virtual std::unique_ptr<ISignal> obtain(size_t sz) = 0;
};


}