﻿// SPDX-FileCopyrightText: 2018 my04337
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
	{
		lsp_require(table.samples() > 0);
		lsp_require(table.channels() == 1);
	}

	sample_type update(parameter_type sampleFreq, parameter_type freq)
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
		if(!mTable) return 0;
		const size_t samples = mTable->samples();
		auto pos = static_cast<size_t>(phase * samples);
		sample_type v = mTable->data(0, pos);
		return v * mVolume;
	}

private:
	const Signal<sample_type>* mTable; // mCycles周期分の信号
	parameter_type mVolume; // 出力ボリューム
	parameter_type mCycles; // テーブルの周期数
	parameter_type mPhase = 0; // 現在の位相 [0, 1)
};

}