#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP
{

// TODO いずれ信号型で固定長バッファからのアロケートを行いたい

/// サンプル型情報 
template<typename sample_type> struct sample_traits {
	// MEMO abs(min) == abs(max)が成り立つようにする
};
template<> struct sample_traits<int8_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int8_t;
	static constexpr sample_type abs_max()noexcept { return 0x7Fi8; }
	static constexpr sample_type normalized_max()noexcept { return +abs_max(); }
	static constexpr sample_type normalized_min()noexcept { return -abs_max(); }
	static constexpr bool need_clipping_on_normalize()noexcept { return true; }
};
template<> struct sample_traits<int16_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int16_t;
	static constexpr sample_type abs_max()noexcept { return 0x7FFFi16; }
	static constexpr sample_type normalized_max()noexcept { return +abs_max(); }
	static constexpr sample_type normalized_min()noexcept { return -abs_max(); }
	static constexpr bool need_clipping_on_normalize()noexcept { return true; }
};
template<> struct sample_traits<int32_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int32_t;
	static constexpr sample_type abs_max()noexcept { return 0x7FFFFFFFi32; }
	static constexpr sample_type normalized_max()noexcept { return +abs_max(); }
	static constexpr sample_type normalized_min()noexcept { return -abs_max(); }
	static constexpr bool need_clipping_on_normalize()noexcept { return true; }
};
template<> struct sample_traits<float> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = float;
	static constexpr sample_type abs_max() noexcept{ return 1.0f; }
	static constexpr sample_type normalized_max()noexcept { return +abs_max(); }
	static constexpr sample_type normalized_min()noexcept { return -abs_max(); }
	static constexpr bool need_clipping_on_normalize()noexcept { return false; }
};
template<> struct sample_traits<double> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = double;
	static constexpr sample_type abs_max()noexcept { return 1.0; }
	static constexpr sample_type normalized_max()noexcept { return +abs_max(); }
	static constexpr sample_type normalized_min()noexcept { return -abs_max(); }
	static constexpr bool need_clipping_on_normalize()noexcept { return false; }
};

// サンプル型であるか否か
template <class T, class = void>
struct is_sample_type : std::false_type {};
template <class T>
struct is_sample_type<T, std::void_t<typename sample_traits<T>::_sample_type_tag>> : std::true_type {};
template<class T>
constexpr bool is_sample_type_v = is_sample_type<T>::value;


// サンプル変換
template<typename Tin, typename Tout, typename Tintermediate=double>
struct SampleConverter
{
	static Tout convert(Tin in_) {
		constexpr auto in_abs_max = static_cast<Tintermediate>(sample_traits<Tin>::abs_max());
		constexpr auto out_abs_max = static_cast<Tintermediate>(sample_traits<Tout>::abs_max());
		constexpr auto amp_rate = out_abs_max / in_abs_max;
		
		const auto in = static_cast<Tintermediate>(in_);
		const auto raw_out = in * amp_rate;

		if constexpr (sample_traits<Tout>::need_clipping_on_normalize()) {
			constexpr out_min = static_cast<Tintermediate>(sample_traits<Tout>::normalized_min());
			constexpr out_max = static_cast<Tintermediate>(sample_traits<Tout>::normalized_max());
			return static_cast<Tout>(std::max(out_min, std::min(raw_out, out_max)));
		} else {
			return static_cast<Tout>(raw_out);
		}
	}
};

// ---

// 信号型であるか否か
template <class T, typename = void>
struct is_signal_type : std::false_type {};
template <class T>
struct is_signal_type<T, std::void_t<typename T::_signal_type_tag>> : std::true_type {};
template<class T>
constexpr bool is_signal_type_v = is_signal_type<T>::value;

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