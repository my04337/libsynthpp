#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Base/Math.hpp>

#include <random>

namespace LSP::Generator {

// 波形テーブルジェネレータ
template<
	sample_typeable sample_type,
	floating_sample_typeable parameter_type = std::conditional_t<floating_sample_typeable<sample_type>, sample_type, float>
>
class WaveTableGenerator final
{
public:
	WaveTableGenerator(SignalView<sample_type> table, parameter_type volume = 1.0f, parameter_type cycles = 1.0f)
		: mTable(table)
		, mVolume(volume)
		, mCycles(cycles)
	{
		lsp_assert(table.frames() > 0);
		lsp_assert(table.channels() == 1);
	}

	sample_type update(uint32_t sampleFreq, parameter_type freq)
	{
		parameter_type phaseDelta = freq / sampleFreq / mCycles;
		sample_type v = peek(mPhase);
		mPhase = Math::floored_division< parameter_type>(mPhase + phaseDelta, 1);
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
	const SignalView<sample_type> mTable; // mCycles周期分の信号
	const parameter_type mVolume; // 出力ボリューム
	const parameter_type mCycles; // テーブルの周期数
	parameter_type mPhase = 0; // 現在の位相 [0, 1)
};

}