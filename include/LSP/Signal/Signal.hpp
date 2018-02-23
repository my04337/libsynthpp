#pragma once

#include <LSP/Base/Base.hpp>

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

/// 信号型の単純な実装
class Signal final
	: public ISignal 
{
public:
	explicit Signal(size_t size);
	virtual ~Signal();

	// データへのポインタを取得します
	virtual float_t* data()override;

	// データ数を取得します。
	virtual size_t size()const override;

protected:
	void allocate(size_t size);

private:
	float_t* mData;
	size_t mSize;
};

// ----------------------------------------------------------------------------

/// 信号ソース
class ISignalSource 
	: non_copy_move
{
public:
	virtual ~ISignalSource() {}

	virtual std::shared_ptr<ISignal> obtain(size_t sz) = 0;
};

/// 信号ソースの単純な実装
class SignalSource final
	: public ISignalSource 
{
public:
	virtual ~SignalSource() {}

	virtual std::shared_ptr<ISignal> obtain(size_t sz) override;
};

}