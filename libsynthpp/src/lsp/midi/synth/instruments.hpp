/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/dsp/wave_table_generator.hpp>

namespace lsp::midi::synth
{

// 楽器情報
class Instruments 
{
public:
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;

	// 波形テーブル一覧

	// 常に0を返すジェネレータを作成します
	static WaveTableGenerator createZeroWaveTable(float volume = 1.f);

	// 正弦波のジェネレータを返します
	static WaveTableGenerator createSquareGenerator(int overtoneOrder = 30, float volume = 1.f);

	// ドラム用ノイズのジェネレータを返します
	static WaveTableGenerator createDrumNoiseGenerator(float volume = 1.f);

};
}
