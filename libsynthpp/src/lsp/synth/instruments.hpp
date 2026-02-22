// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/dsp/wave_table_generator.hpp>

namespace lsp::synth
{

// 楽器情報
class Instruments  final
{
public:
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;

	// 波形テーブルを予め初期化します
	static void prepareWaveTable();

	// 常に0を返すジェネレータを作成します
	static WaveTableGenerator createZeroWaveTable(float volume = 1.f);

	// 矩形波のジェネレータを返します
	static WaveTableGenerator createSquareGenerator(float volume = 1.f);

	// 正弦波のジェネレータを返します
	static WaveTableGenerator createSineGenerator(float volume = 1.f);

	// 三角波のジェネレータを返します
	static WaveTableGenerator createTriangleGenerator(float volume = 1.f);

	// のこぎり波のジェネレータを返します
	static WaveTableGenerator createSawtoothGenerator(float volume = 1.f);

	// ドラム用ノイズのジェネレータを返します
	static WaveTableGenerator createDrumNoiseGenerator(float volume = 1.f);

};
}
