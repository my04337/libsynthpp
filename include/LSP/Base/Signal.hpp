#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Logging.hpp>

namespace LSP
{

// TODO いずれ信号型で固定長バッファからのアロケートを行いたい

/// サンプル型情報 
template<typename sample_type> struct sample_traits {
	// MEMO abs(min) == abs(max)が成り立つようにすること
};
template<> struct sample_traits<int8_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int8_t;
	static constexpr sample_type abs_max = 0x7Fi8;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<int16_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int16_t;
	static constexpr sample_type abs_max = 0x7FFFi16; 
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<int32_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int32_t;
	static constexpr sample_type abs_max = 0x7FFFFFFFi32;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<float> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = float;
	static constexpr sample_type abs_max = 1.0f;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<double> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = double;
	static constexpr sample_type abs_max = 1.0;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};

// サンプル型であるか否か
template <class T, class = void>
struct is_sample_type : std::false_type {};
template <class T>
struct is_sample_type<T, std::void_t<typename sample_traits<T>::_sample_type_tag>> : std::true_type {};
template<class T>
constexpr bool is_sample_type_v = is_sample_type<T>::value;

// ---

/// 信号型
template<
	typename sample_type_,
	class = std::enable_if_t<is_sample_type_v<sample_type_>>
>
class Signal final
	: non_copy
{
public:
	using sample_type = sample_type_;

public:
	Signal(size_t size)
		: Signal(1, size)
	{
	}
	Signal(size_t channels, size_t size)
		: mSize(size)
	{
		for (size_t ch = 0; ch < channels; ++ch) {
			mData.emplace_back(std::make_unique<sample_type[]>(size));
		}
	}

	~Signal() {}

	Signal(Signal&&)noexcept = default;
	Signal& operator=(Signal&&)noexcept = default;
	
	// チャネル数を取得します
	size_t channels()const noexcept { return mData.size(); }

	// データへのポインタを取得します
	sample_type* data()const noexcept { return mData[0]; }
	sample_type* data(size_t ch)const noexcept { return mData[ch].get(); }

	// データ数を取得します。
	size_t size()const noexcept { return mSize; }
	size_t length()const noexcept { return mSize; }	
	
private:
	const size_t mSize;
	std::vector<std::unique_ptr<sample_type[]>> mData;
};


}