#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Base/Math.hpp>
#include <LSP/Filter/Requantizer.hpp>
#include <LSP/Filter/Normalizer.hpp>

#include <random>

namespace LSP::Generator {

// 波形種別
enum class WaveFormType
{
	Ground,			// 常に0を出力
	Sin,			// 正弦波
	Saw,			// 鋸波
	Triangle,		// 三角波
	Square,			// 矩形波
	WhiteNoise,		// ホワイトノイズ
	BrownNoise,		// ブラウンノイズ
};

// ファンクションジェネレータ : 複数種類の波形生成
template<
	typename sample_type,
	typename parameter_type = sample_type,
	class = std::enable_if_t<
		is_sample_type_v<sample_type> && is_floating_point_sample_type_v<parameter_type>
	>
>
class FunctionGenerator final
{
public:
	FunctionGenerator()
		: mType(WaveFormType::Ground)
		, mPhase(0)
		, mDutyRate(0)
		, mSamplePerPhase(0)
		, mRandomEngine(std::random_device()())
		, mUniDist(sample_traits<parameter_type>::normalized_min, sample_traits<parameter_type>::normalized_max)
	{
	}

	void setGround()noexcept 
	{
		mType = WaveFormType::Ground;
		mSamplePerPhase = 0;
	}
	void setSinWave(uint32_t sampleFreq, parameter_type freq)noexcept 
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Sin;
		mSamplePerPhase = 2.0f * PI<parameter_type> * (freq_ / sampleFreq);
		mPhase = 0;
	}
	void setSawWave(uint32_t sampleFreq, parameter_type freq)noexcept 
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Saw;
		mSamplePerPhase = 2.0f * PI<parameter_type> * (freq_ / sampleFreq);
		mPhase = 0;
	}
	void setTriangleWave(uint32_t sampleFreq, parameter_type freq)noexcept 
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Triangle;
		mSamplePerPhase = 2.0f * PI<parameter_type> * (freq_ / sampleFreq);
		mPhase = 0;
	}
	void setSquareWave(uint32_t sampleFreq, parameter_type freq, parameter_type duty=PI<parameter_type>)noexcept 
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Square;
		mSamplePerPhase = 2.0f * PI<parameter_type> * (freq_ / sampleFreq);
		mDutyRate = duty;
		mPhase = 0;
	}
	void setWhiteNoise()noexcept 
	{
		mType = WaveFormType::WhiteNoise;
		mSamplePerPhase = 0;
	}
	void setBrownNoise()noexcept 
	{
		mType = WaveFormType::BrownNoise;
		mSamplePerPhase = 0;
		mBrownNoisePrevLevel = 0;
	}


	sample_type update() 
	{
		constexpr auto normalize = Filter::Normalizer();
		constexpr auto to_sample_type = Filter::Requantizer<sample_type>();
		constexpr auto period = 2.0f * PI<parameter_type>; // 一周期 : 2π
		constexpr auto half_period = PI<parameter_type>; // 一周期 : 2π

		sample_type s = 0;
		switch (mType) {
		case WaveFormType::Ground: 
			// グラウンド : 常に0
			s = 0;
			break;
		case WaveFormType::Sin:	
			// 正弦波
			s = to_sample_type(std::sin(mPhase));
			break;
		case WaveFormType::Saw:	
			// 鋸波
			s = to_sample_type(-1.0f + 2.0f*(mPhase / period));
			break;
		case WaveFormType::Triangle:	
			// 三角波
			if(mPhase < half_period) {
				s = to_sample_type(-1.0f + 2.0f*(mPhase / half_period));
			} else {
				s = to_sample_type(+1.0f - 2.0f*((mPhase - half_period) / half_period));
			}
			break;
		case WaveFormType::Square:	
			// 矩形波
			if(mPhase < mDutyRate) {
				s = to_sample_type(+1.0f);
			} else {
				s = to_sample_type(-1.0f);
			}
			break;
		case WaveFormType::WhiteNoise:
			// ホワイトノイズ
			s = to_sample_type(mUniDist(mRandomEngine));
			break;
		case WaveFormType::BrownNoise: {
			// ブラウンノイズ
			auto delta = mUniDist(mRandomEngine)/100; // 概ね20dB下げると丁度良い波形が生成できる。 理由は不明
			mBrownNoisePrevLevel = normalize(mBrownNoisePrevLevel + delta);
			s = to_sample_type(mBrownNoisePrevLevel);
		}	break;
		}

		mPhase = std::fmod(mPhase + mSamplePerPhase, period);
		return s;
	}

private:
	WaveFormType mType;
	parameter_type mPhase; // 現在の位相
	parameter_type mDutyRate;	// デューティー比
	parameter_type mSamplePerPhase; // 1サンプル当たりの位相角(rad)

	std::mt19937 mRandomEngine;
	std::uniform_real_distribution<parameter_type> mUniDist; // 一様分布
	parameter_type mBrownNoisePrevLevel;
	
};

}