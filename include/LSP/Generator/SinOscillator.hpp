#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Base/Math.hpp>

namespace LSP::Generator {

// 正弦波ジェネレータ
template<
	typename sample_type,
	class = std::enable_if_t<
	std::is_arithmetic_v<sample_type>
	>
>
class SinOscillator final
{
public:
	SinOscillator(uint32_t sampleFreq, double initialOscillationFreq = 0)
		: mSampleFreq(sampleFreq)
	{
		setOscillationFreq(initialOscillationFreq);
	}

	double oscillationFreq()const noexcept 
	{
		return mOscillationFreq; 
	}

	void setOscillationFreq(double freq)noexcept 
	{
		mOscillationFreq = std::abs(freq); 
		mSamplePerPhase = 2.0 * PI<double> * (freq / mSampleFreq);
	}

	sample_type generate() 
	{
		auto s = std::sin(mPhase);
		mPhase = std::fmod(mPhase + mSamplePerPhase, 2.0 * PI<double>);
		return SampleFormatConverter<decltype(s), sample_type>::convert(s);
	}

	Signal<sample_type> generate(size_t sz) 
	{
		Signal<sample_type> sig(sz);
		auto data = sig.data();
		
		for(size_t i=0; i<sz; ++i) data[i] = generate();

		return sig; // NRVO
	}

private:
	const uint32_t mSampleFreq;
	float mOscillationFreq;
	double mPhase;
	double mSamplePerPhase; // 1サンプル当たりの位相角(rad)

};

}