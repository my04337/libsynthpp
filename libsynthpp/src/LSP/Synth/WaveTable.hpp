#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>

namespace LSP::Synth
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
	};

public:
	WaveTable();

	// 登録済みカスタム波形をリセットします
	void reset();

	// カスタム波形を登録します
	size_t add(Signal<float>&& wav);

	// 波形を取得します
	SignalView<float> get(size_t id)const;


private:
	void add(size_t id, Signal<float>&& wav);

private:
	std::unordered_map<size_t, Signal<float>> mWaveTable;
	size_t mNextCustomWaveId = CustomWaveIdBegin;
};
}
