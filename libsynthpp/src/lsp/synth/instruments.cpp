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
		createSquareGenerator(0.f);
		createSineGenerator(0.f);
		createTriangleGenerator(0.f);
		createSawtoothGenerator(0.f);
		createDrumNoiseGenerator(0.f);
	});
}

// 矩形波のジェネレータを返します
auto Instruments::createSquareGenerator(float volume)
	-> WaveTableGenerator
{
	static const auto table = []() -> Signal<float> {
		constexpr size_t samples = 512;
		auto table = Signal<float>::allocate(samples);
		auto data = table.data();
		for(size_t i = 0; i < samples; ++i) {
			data[i] = (i < samples / 2) ? 1.0f : -1.0f;
		}
		return table;
	}();

	return WaveTableGenerator(table, volume);
}

// 正弦波のジェネレータを返します
auto Instruments::createSineGenerator(float volume)
	-> WaveTableGenerator
{
	using FunctionGenerator = dsp::FunctionGenerator<float>;

	static const auto table = []() -> Signal<float> {
		constexpr size_t samples = 512;
		auto table = Signal<float>::allocate(samples);
		auto data = table.data();
		FunctionGenerator fg;
		fg.setSinWave(static_cast<float>(samples), 1.0f);
		for(size_t i = 0; i < samples; ++i) {
			data[i] = fg.update();
		}
		return table;
	}();

	return WaveTableGenerator(table, volume);
}

// 三角波のジェネレータを返します
auto Instruments::createTriangleGenerator(float volume)
	-> WaveTableGenerator
{
	static const auto table = []() -> Signal<float> {
		constexpr size_t samples = 512;
		auto table = Signal<float>::allocate(samples);
		auto data = table.data();
		for(size_t i = 0; i < samples; ++i) {
			// 0→+1→0→-1→0 の三角波
			float phase = static_cast<float>(i) / static_cast<float>(samples);
			if(phase < 0.25f) {
				data[i] = phase * 4.0f;
			} else if(phase < 0.75f) {
				data[i] = 2.0f - phase * 4.0f;
			} else {
				data[i] = phase * 4.0f - 4.0f;
			}
		}
		return table;
	}();

	return WaveTableGenerator(table, volume);
}

// のこぎり波のジェネレータを返します
auto Instruments::createSawtoothGenerator(float volume)
	-> WaveTableGenerator
{
	static const auto table = []() -> Signal<float> {
		constexpr size_t samples = 512;
		auto table = Signal<float>::allocate(samples);
		auto data = table.data();
		for(size_t i = 0; i < samples; ++i) {
			// -1→+1 の線形変化
			data[i] = 2.0f * static_cast<float>(i) / static_cast<float>(samples) - 1.0f;
		}
		return table;
	}();

	return WaveTableGenerator(table, volume);
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