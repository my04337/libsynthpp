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
	WaveTableGenerator(SignalView<sample_type> table)
		: mTable(table)
	{
		lsp_assert(table.frames() > 0);
		lsp_assert(table.channels() == 1);
	}

	sample_type update(uint32_t sampleFreq, parameter_type freq)
	{
		parameter_type phaseDelta = freq / sampleFreq;
		return update(phaseDelta);
	}
	sample_type update(parameter_type phaseDelta)
	{
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
		return v;
	}

private:
	const SignalView<sample_type> mTable; // 1周期分の信号
	parameter_type mPhase = 0;
};

}