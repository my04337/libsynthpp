#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Base/Math.hpp>

#include <random>

namespace LSP::Generator {


enum class NoiseColor
{
	White,		// ホワイトノイズ : 一様分布
};

// ノイズジェネレータ
template<
	typename sample_type,
	NoiseColor noise_color,
	class = std::enable_if_t<
	std::is_arithmetic_v<sample_type>
	>
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
		}
	}
	
	sample_type generate() 
	{
		using conv = SampleFormatConverter<double, sample_type>;
		if constexpr(noise_color == NoiseColor::White) {
			return conv()(mUniDist(mRandomEngine));
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

	std::mt19937 mRandomEngine;
	std::uniform_real_distribution<double> mUniDist;
	std::variant<WhiteNoiseParams> mParams;

	const uint32_t mSampleFreq;
};

}