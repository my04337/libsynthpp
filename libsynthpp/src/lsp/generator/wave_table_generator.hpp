/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>

#include <random>

namespace lsp::generator {

// 波形テーブルジェネレータ
template<
	class sample_type,
	std::floating_point parameter_type = std::conditional_t<std::is_floating_point_v<sample_type>, sample_type, float>
> requires std::signed_integral<sample_type> || std::floating_point<sample_type>
class WaveTableGenerator final

{
public:
	WaveTableGenerator(const Signal<sample_type>& table, parameter_type volume = 1.0f, parameter_type cycles = 1.0f)
		: mTable(table)
		, mVolume(volume)
		, mCycles(cycles)
	{
		require(table.frames() > 0);
		require(table.channels() == 1);
	}

	sample_type update(uint32_t sampleFreq, parameter_type freq)
	{
		parameter_type phaseDelta = freq / sampleFreq / mCycles;
		sample_type v = peek(mPhase);
		mPhase = math::floored_division< parameter_type>(mPhase + phaseDelta, 1);
		return v;
	}

private:
	sample_type peek()const 
	{
		return peek(mPhase);
	}
	sample_type peek(parameter_type phase)const
	{
		const size_t frames = mTable.frames();
		auto pos = static_cast<size_t>(phase * frames);
		sample_type v = mTable.frame(pos)[0];
		return v * mVolume;
	}

private:
	const Signal<sample_type>& mTable; // mCycles周期分の信号
	const parameter_type mVolume; // 出力ボリューム
	const parameter_type mCycles; // テーブルの周期数
	parameter_type mPhase = 0; // 現在の位相 [0, 1)
};

}