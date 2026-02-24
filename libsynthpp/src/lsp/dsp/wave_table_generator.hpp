// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>

#include <random>

namespace lsp::dsp {

// 波形テーブルジェネレータ
template<
	class sample_type,
	std::floating_point parameter_type = std::conditional_t<std::is_floating_point_v<sample_type>, sample_type, float>
> requires std::signed_integral<sample_type> || std::floating_point<sample_type>
class WaveTableGenerator final

{
public:
	WaveTableGenerator()
		: mTable(nullptr)
		, mVolume(0)
		, mCycles(1)
	{}
	WaveTableGenerator(const Signal<sample_type>& table, parameter_type volume = 1.0f, parameter_type cycles = 1.0f)
		: mTable(&table)
		, mVolume(volume)
		, mCycles(cycles)
		, mPerceptualNorm(computePerceptualNorm(table, volume, cycles))
	{
		lsp_require(table.frames() > 0);
		lsp_require(table.channels() == 1);
	}

	// 人間の聴覚上の音量を均一化するための係数を返します
	// 正弦波を基準(1.0)として、波形のRMSに基づいて算出されます
	parameter_type perceptualNormalization() const noexcept { return mPerceptualNorm; }

	sample_type update(parameter_type sampleFreq, parameter_type freq)
	{
		parameter_type phaseDelta = freq / sampleFreq / mCycles;
		sample_type v = peek(mPhase);
		mPhase = math::floored_division< parameter_type>(mPhase + phaseDelta, 1);
		return v;
	}

private:
	// 1周期分の波形データからRMSベースの知覚音量正規化係数を算出します
	// 正弦波(RMS = 1/√2)を基準とし、実効出力(テーブル値 × volume)のRMSとの比を返します
	static parameter_type computePerceptualNorm(const Signal<sample_type>& table, parameter_type volume, parameter_type cycles)
	{
		// 1周期分のフレーム数を算出
		size_t totalFrames = table.frames();
		size_t oneCycleFrames = static_cast<size_t>(static_cast<parameter_type>(totalFrames) / cycles);
		if(oneCycleFrames == 0) oneCycleFrames = totalFrames;

		// 実効出力のRMSを算出 (テーブル値 × volume)
		parameter_type sumSq = 0;
		for(size_t i = 0; i < oneCycleFrames; ++i) {
			parameter_type v = static_cast<parameter_type>(table.frame(i)[0]) * volume;
			sumSq += v * v;
		}
		parameter_type rms = static_cast<parameter_type>(std::sqrt(sumSq / static_cast<parameter_type>(oneCycleFrames)));

		// 基準RMS: 振幅1.0・volume 1.0の正弦波 = 1/√2
		constexpr parameter_type refRms = parameter_type(1) / std::numbers::sqrt2_v<parameter_type>;

		return (rms > parameter_type(0)) ? (refRms / rms) : parameter_type(1);
	}

	sample_type peek()const 
	{
		return peek(mPhase);
	}
	sample_type peek(parameter_type phase)const
	{
		if(!mTable) return 0;
		const size_t frames = mTable->frames();
		auto pos = static_cast<size_t>(phase * frames);
		sample_type v = mTable->frame(pos)[0];
		return v * mVolume;
	}

private:
	const Signal<sample_type>* mTable; // mCycles周期分の信号
	parameter_type mVolume; // 出力ボリューム
	parameter_type mCycles; // テーブルの周期数
	parameter_type mPhase = 0; // 現在の位相 [0, 1)
	parameter_type mPerceptualNorm = 1; // 知覚音量正規化係数 (正弦波基準)
};

}