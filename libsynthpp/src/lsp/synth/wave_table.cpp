﻿#include <lsp/synth/wave_table.hpp>
#include <lsp/generator/function_generator.hpp>
#include <lsp/filter/biquadratic_filter.hpp>

using namespace LSP;
using namespace LSP::Synth;

WaveTable::WaveTable()
{
	reset();
}
size_t WaveTable::add(Signal<float>&& wav, float preAmp, float cycles)
{
	size_t id = mNextCustomWaveId++;
	add(id, std::move(wav), preAmp, cycles);
	return id;
}
void WaveTable::add(size_t id, Signal<float>&& wav, float preAmp, float cycles)
{
	Assertion::require(wav.channels() == 1);

	if (preAmp < 0) {
		// MEMO 実効値の2乗=パワーを用いて正規化すると、それらしい音量で揃う(ラウドネスは考慮していないので注意)
		float rms = calcRMS(wav);
		preAmp = mBaseRMS * mBaseRMS / (rms * rms + 1.0e-8f) * abs(preAmp);
	}
	mWaveTable.insert_or_assign(id, std::make_tuple(std::move(wav), preAmp, cycles));
}

LSP::Generator::WaveTableGenerator<float> WaveTable::get(size_t id)const
{
	auto found = mWaveTable.find(id);
	if (found == mWaveTable.end()) {
		found = mWaveTable.find(Preset::Ground);
		Assertion::check(found != mWaveTable.end());
	}

	auto& [wave, preAmp, cycles] = found->second;

	return LSP::Generator::WaveTableGenerator<float>(wave, preAmp, cycles);
}

void WaveTable::reset()
{
	using FunctionGenerator = LSP::Generator::FunctionGenerator<float>;
	using BiquadraticFilter = LSP::Filter::BiquadraticFilter<float>;

	mNextCustomWaveId = CustomWaveIdBegin;
	mWaveTable.clear();

	// Ground
	{
		auto sig = Signal<float>::allocate(1);
		*sig.data() = 0;
		add(Preset::Ground, std::move(sig), 0);
	}
	// SinWave
	{
		constexpr size_t frames = 512;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setSinWave(frames, 1);
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		mBaseRMS = calcRMS(sig);
		add(Preset::SinWave, std::move(sig), 1);
	}
	// SquareWave50
	{
		constexpr size_t frames = 512;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setSquareWave(frames, 1);
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		add(Preset::SquareWave50, std::move(sig));
	}
	// SquareWave33
	{
		constexpr size_t frames = 512;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setSquareWave(frames, 1, Math::PI<float>/1.5f);
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		add(Preset::SquareWave33, std::move(sig));
	}
	// SquareWave25
	{
		constexpr size_t frames = 512;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setSquareWave(frames, 1, Math::PI<float>/2);
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		add(Preset::SquareWave25, std::move(sig));
	}
	// WhiteNoise
	{
		// MEMO サンプリング周波数よりサンプル数を大きくしないと、金属音のような規則性のある音が混ざることに注意
		constexpr size_t frames = 16384; 
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setWhiteNoise();
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		add(Preset::WhiteNoise, std::move(sig));
	}
	// DrumNoise : LSP用デフォルトドラム波形
	{
		int frames = 131072;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setWhiteNoise();
		std::array<BiquadraticFilter, 5> bqfs;
		bqfs[0].setLopassParam(44100.f, 4000.f, 0.5f); // 不要高周波を緩やかにカットオフ
		bqfs[1].setLopassParam(44100.f, 4000.f, 0.5f); // (同上)
		bqfs[2].setLopassParam(44100.f, 3000.f, 0.5f); // (同上)
		bqfs[3].setLopassParam(44100.f, 2000.f, 0.5f); // (同上)
		bqfs[4].setLopassParam(44100.f, 1000.f, 1.0f); // 基本となる音程
		for(size_t i = 0; i < frames*2; i++) {		
			// 波形が安定するまで読み捨てる
			float s = fg.update();
			for(auto& bqf : bqfs) s = bqf.update(s);
		}
		for(size_t i = 0; i < frames; ++i) {
			float s = fg.update();
			for(auto& bqf : bqfs) s = bqf.update(s);
			sig.frame(i)[0] = s;
		}
		add(Preset::DrumNoise, std::move(sig), -0.30f, 62.5f);
	}
}

float WaveTable::calcRMS(SignalView<float> wav)
{
	Assertion::require(wav.channels() == 1);

	// 信号の2乗平均値を取る
	const size_t frames = wav.frames();
	float p = 0;
	for (size_t i = 0; i < frames; ++i) {
		p += wav.frame(i)[0] * wav.frame(i)[0];
	}
	p /= frames;
	return std::sqrt(p);
}