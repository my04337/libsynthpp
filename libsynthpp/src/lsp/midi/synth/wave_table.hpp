#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/signal.hpp>
#include <lsp/generator/wave_table_generator.hpp>

namespace lsp::midi::synth
{

// 波形テーブル
class WaveTable 
{
public:
	static constexpr size_t CustomWaveIdBegin = 1024;

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

	// 登録済みカスタム波形をリセットします
	void reset();

	// カスタム波形を登録します
	size_t add(Signal<float>&& wav, float preAmp = -1, float cycles = 1);

	// 波形テーブルジェネレータを取得します
	generator::WaveTableGenerator<float> get(size_t id)const;


private:
	void add(size_t id, Signal<float>&& wav, float preAmp = -1, float cycles = 1);
	static float calcRMS(SignalView<float> wav);

private:
	std::unordered_map<size_t, std::tuple<Signal<float>, /*preAmp*/float, /*cycles*/float>> mWaveTable;
	size_t mNextCustomWaveId = CustomWaveIdBegin;
	float mBaseRMS; // 基準となる実効値
};
}
