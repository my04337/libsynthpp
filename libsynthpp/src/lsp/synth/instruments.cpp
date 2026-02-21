// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#include <lsp/synth/instruments.hpp>
#include <lsp/dsp/function_generator.hpp>
#include <lsp/dsp/biquadratic_filter.hpp>

using namespace lsp;
using namespace lsp::synth;

auto Instruments::createZeroWaveTable(float volume) 
	-> WaveTableGenerator
{
	static const auto table = []() ->Signal<float> {
		auto table = Signal<float>::allocate(1);
		*table.data() = 0;
		return table;
	}();

	return WaveTableGenerator(table, volume);
}

// 波形テーブルを予め初期化します
void Instruments::prepareWaveTable()
{
	static std::once_flag once;
	std::call_once(once,[] {
		createSquareGenerator(0, 0.f);
		createDrumNoiseGenerator(0.f);
	});
}

// 矩形波（バンドリミテッド）のジェネレータを返します
auto Instruments::createSquareGenerator(int overtoneOrder, float volume)
	-> WaveTableGenerator
{
	static constexpr size_t MAX_OVERTONE_ORDER = 50;
	using FunctionGenerator = dsp::FunctionGenerator<float>;

	static const auto tables = []() ->std::array<Signal<float>, MAX_OVERTONE_ORDER + 1> {
		std::array <Signal<float>, MAX_OVERTONE_ORDER + 1> tables;
		constexpr size_t samples = 512;

		for(uint32_t order = 0; order < 51; ++order) {
			auto table = Signal<float>::allocate(samples);
			auto data = table.data();
			std::fill(data, data + samples, 0.f);
			for(uint32_t o = 0; o < order; ++o) {
				FunctionGenerator fg;
				fg.setSinWave(static_cast<float>(samples), static_cast<float>(1 + o * 2));
				for(size_t i = 0; i < samples; ++i) {
					data[i] += fg.update() / (1 + o * 2);
				}
			}
			tables[order] = std::move(table);
		}
		return tables;
	}();

	lsp_require(overtoneOrder >= 0 && static_cast<size_t>(overtoneOrder) < MAX_OVERTONE_ORDER + 1);
	return WaveTableGenerator(tables[static_cast<size_t>(overtoneOrder)], volume);
}

// ドラム用ノイズのジェネレータを返します
auto Instruments::createDrumNoiseGenerator(float volume)
	-> WaveTableGenerator
{
	using FunctionGenerator = dsp::FunctionGenerator<float>;
	using BiquadraticFilter = dsp::BiquadraticFilter<float>;
	static constexpr size_t samples = 131072;

	static const auto [table, preAmp] = []() -> std::tuple<Signal<float>, float> {
		auto table = Signal<float>::allocate(samples);
		auto data = table.data();
		FunctionGenerator fg;
		fg.setWhiteNoise();
		std::array<BiquadraticFilter, 6> bqfs;
		bqfs[0].setLopassParam(44100, 4000.f, 1.0f); // 不要高周波を緩やかにカットオフ
		bqfs[1].setLopassParam(44100, 4000.f, 0.5f); // (同上)
		bqfs[2].setLopassParam(44100, 3000.f, 0.5f); // (同上)
		bqfs[3].setLopassParam(44100, 2000.f, 0.5f); // (同上)
		bqfs[4].setLopassParam(44100, 1000.f, 1.0f); // 基本となる高さ
		for(size_t i = 0; i < samples * 2; i++) {
			// 波形が安定するまで読み捨てる
			float s = fg.update();
			for(auto& bqf : bqfs) s = bqf.update(s);
		}
		for(size_t i = 0; i < samples; ++i) {
			float s = fg.update();
			for(auto& bqf : bqfs) s = bqf.update(s);
			data[i] = s;
		}
		auto preAmp = 10.0f;
		return std::make_tuple(std::move(table), preAmp);
	}();

	return WaveTableGenerator(table, preAmp * volume, 62.5f);
}