// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>

#include <random>

namespace lsp::dsp {

// 波形種別
enum class WaveFormType
{
	Ground,			// 常に0を出力
	Sin,			// 正弦波
	Saw,			// 鋸波
	Triangle,		// 三角波
	Square,			// 矩形波
	WhiteNoise,		// ホワイトノイズ
};

// ファンクションジェネレータ : 複数種類の波形生成
template<
	class sample_type,
	std::floating_point parameter_type = std::conditional_t<std::is_floating_point_v<sample_type>, sample_type, float>
>requires std::signed_integral<sample_type> || std::floating_point<sample_type>
class FunctionGenerator final
{
public:
	FunctionGenerator()
		: mType(WaveFormType::Ground)
		, mPhase(0)
		, mDutyRate(0)
		, mPhasePerSample(0)
		, mRandomEngine(std::random_device()())
		, mUniDist(sample_traits<parameter_type>::min, sample_traits<parameter_type>::max)
	{
	}

	void setGround()noexcept 
	{
		mType = WaveFormType::Ground;
		mPhasePerSample = 0;
	}
	void setSinWave(parameter_type sampleFreq, parameter_type freq, bool keepPhase = false)noexcept 
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Sin;
		mPhasePerSample = 2.0f * math::PI<parameter_type> * (freq_ / sampleFreq);
		if(!keepPhase) mPhase = 0;
	}
	void setSawWave(parameter_type sampleFreq, parameter_type freq, bool keepPhase = false)noexcept
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Saw;
		mPhasePerSample = 2.0f * math::PI<parameter_type> * (freq_ / sampleFreq);
		if (!keepPhase) mPhase = 0;
	}
	void setTriangleWave(parameter_type sampleFreq, parameter_type freq, bool keepPhase = false)noexcept
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Triangle;
		mPhasePerSample = 2.0f * math::PI<parameter_type> * (freq_ / sampleFreq);
		if (!keepPhase) mPhase = 0;
	}
	void setSquareWave(parameter_type sampleFreq, parameter_type freq, parameter_type duty=math::PI<parameter_type>, bool keepPhase = false)noexcept
	{
		auto freq_ = std::abs(freq);  // 負の位相はこの実装では対応不可
		mType = WaveFormType::Square;
		mPhasePerSample = 2.0f * math::PI<parameter_type> * (freq_ / sampleFreq);
		mDutyRate = duty;
		if (!keepPhase) mPhase = 0;
	}
	void setWhiteNoise()noexcept 
	{
		mType = WaveFormType::WhiteNoise;
		mPhasePerSample = 0;
	}


	sample_type update() 
	{
		constexpr auto period = 2.0f * math::PI<parameter_type>; // 一周期 : 2π
		constexpr auto half_period = math::PI<parameter_type>; // 半周期 : π

		sample_type s = 0;
		switch (mType) {
		case WaveFormType::Ground: 
			// グラウンド : 常に0
			s = 0;
			break;
		case WaveFormType::Sin:	
			// 正弦波
			s = requantize<sample_type>(std::sin(mPhase));
			break;
		case WaveFormType::Saw:	
			// 鋸波
			s = requantize<sample_type>(-1.0f + 2.0f*(mPhase / period));
			break;
		case WaveFormType::Triangle:	
			// 三角波
			if(mPhase < half_period) {
				s = requantize<sample_type>(-1.0f + 2.0f*(mPhase / half_period));
			} else {
				s = requantize<sample_type>(+1.0f - 2.0f*((mPhase - half_period) / half_period));
			}
			break;
		case WaveFormType::Square:	
			// 矩形波
			if(mPhase < mDutyRate) {
				s = requantize<sample_type>(+1.0f);
			} else {
				s = requantize<sample_type>(-1.0f);
			}
			break;
		case WaveFormType::WhiteNoise:
			// ホワイトノイズ
			s = requantize<sample_type>(mUniDist(mRandomEngine));
			break;
		}

		mPhase = math::floored_division(mPhase + mPhasePerSample, period);
		return s;
	}

private:
	WaveFormType mType;
	parameter_type mPhase; // 現在の位相 [0, 1)
	parameter_type mDutyRate;	// デューティー比
	parameter_type mPhasePerSample; // 1サンプル当たりの位相角(rad)

	std::mt19937 mRandomEngine;
	std::uniform_real_distribution<parameter_type> mUniDist; // 一様分布
	parameter_type mBrownNoisePrevLevel;
	
};

}