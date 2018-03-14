#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP
{

// TODO いずれ信号型で固定長バッファからのアロケートを行いたい

// 信号型であるか否か
template <class T, typename = void>
struct is_signal_type : std::false_type {};
template <class T>
struct is_signal_type<T, void_t<typename T::_signal_type_tag>> : std::true_type {};
template<class T>
constexpr bool is_signal_type_v = is_signal_type<T>::value;




/// 信号型
template<
	typename sample_type_,
	class = std::enable_if_t<std::is_arithmetic_v<sample_type_>>
>
class Signal final
	: non_copy
{
public:
	using sample_type = sample_type_;
	using _signal_type_tag = void; // for SFINAE

public:
	explicit Signal(size_t size)
		: mData(std::make_unique<sample_type[]>(size))
		, mSize(size)
	{}

	~Signal() {}

	// データへのポインタを取得します
	sample_type* data()const noexcept { return mData.get(); }

	// データ数を取得します。
	size_t size()const noexcept { return mSize; }
	
private:
	std::unique_ptr<sample_type[]> mData;
	size_t mSize;
};

// ----------------------------------------------------------------------------

/// 信号ソース
template<
	typename signal_type,
	class = std::enable_if_t<is_signal_type_v<signal_type>>
>
class SignalSource final
	: non_copy_move 
{
public:
	~SignalSource() {}

	std::shared_ptr<signal_type> obtain(size_t sz) { return std::make_shared<signal_type>(sz); }
};

}