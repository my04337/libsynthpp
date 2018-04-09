#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Base/Math.hpp>
#include <LSP/Filter/Normalizer.hpp>

#include <random>

namespace LSP::Generator {


enum class NoiseColor
{
	White,		// ホワイトノイズ : 一様分布
	Brown,		// ブラウンノイズ : 1/f2 , -6db/Oct, ホワイトノイズの積分
};

// ノイズジェネレータ
template<
	typename sample_type,
	NoiseColor noise_color,
	class = std::enable_if_t<is_sample_type_v<sample_type>>
>
class NoiseGenerator final
{
public:
	NoiseGenerator(uint32_t sampleFreq)
		: mSampleFreq(sampleFreq)
		, mRandomEngine(std::random_device()())
		, mUniDist(sample_traits<double>::normalized_min, sample_traits<double>::normalized_max)
	{
		if constexpr(noise_color == NoiseColor::White) {
			// do-nothing
		} else if constexpr(noise_color == NoiseColor::Brown) {
			mParams = std::make_tuple<double>(0);
		}
	}
	
	sample_type generate() 
	{
		using requantize = Filter::Requantizer<double, sample_type>;
		using norm_double = Filter::Normalizer<double>;
		if constexpr(noise_color == NoiseColor::White) {
			return requantize()(mUniDist(mRandomEngine));
		} else if constexpr(noise_color == NoiseColor::Brown) {
			auto& v = std::get<double>(std::get<BrownNoiseParams>(mParams));
			auto delta = mUniDist(mRandomEngine)/100;
			v = norm_double()(v + delta);
			return requantize()(v);
		} else {
			return static_cast<sample_type>(0);
		}
	}

	Signal<sample_type> generate(size_t sz) 
	{
		Signal<sample_type> sig(sz);
		auto data = sig.data();

		for(size_t i=0; i<sz; ++i) data[i] = generate();

		return sig; // NRVO
	}

private:
	using WhiteNoiseParams = std::tuple<>;
	using BrownNoiseParams = std::tuple<double>;

	std::mt19937 mRandomEngine;
	std::uniform_real_distribution<double> mUniDist;
	std::variant<WhiteNoiseParams, BrownNoiseParams> mParams;

	const uint32_t mSampleFreq;
};

}