/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/generator/wave_table_generator.hpp>

namespace lsp::midi::synth
{

// 波形テーブルセット
class WaveTable 
{
public:
	struct Preset
	{
		// 波形 : 常に出力0
		static constexpr size_t Ground = 0;
		// 波形 : 正弦波
		static constexpr size_t SinWave = 1;
		// 波形 : 矩形波(デューティー比0.5)
		static constexpr size_t SquareWave = 2;
		static constexpr size_t SquareWave50 = SquareWave;
		// 波形 : 矩形波(デューティー比0.33)
		static constexpr size_t SquareWave33 = 3;
		// 波形 : 矩形波(デューティー比0.25)
		static constexpr size_t SquareWave25 = 4;

		// 波形 : ホワイトノイズ
		static constexpr size_t WhiteNoise = 100;
		// 波形 : ドラム用ノイズ
		static constexpr size_t DrumNoise = 200;
	};

public:
	WaveTable();

	// 全ての波形をリセットします
	void reset();

	// プリセットの波形をセットします
	void loadPreset();

	// 波形テーブルを登録します
	void add(size_t id, Signal<float>&& wav, float preAmp = -1, float cycles = 1);

	// 波形テーブルジェネレータを取得します
	generator::WaveTableGenerator<float> createWaveGenerator(size_t id, float volume = 1.f)const;


private:
	std::unordered_map<size_t, std::tuple<Signal<float>, /*preAmp*/float, /*cycles*/float>> mWaveTable;
};
}
