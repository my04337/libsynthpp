#pragma once

#include <LSP/Signal/signal.hpp>

namespace LSP
{

/// 単純な信号型
class Signal final
	: public ISignal 
{
public:
	Signal();
	explicit Signal(size_t size);

	// データへのポインタを取得します
	virtual float_t* data()override;

	// データ数を取得します。
	virtual size_t size()const override;

private:
	std::vector<float_t> mData;
};

/// 単純な信号ソース
class SignalSource final
	: public ISignalSource 
{
public:
	virtual ~SignalSource() {}

	virtual std::unique_ptr<ISignal> obtain(size_t sz) override;
};

}