/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/midi/synth/wave_table.hpp>
#include <lsp/generator/function_generator.hpp>
#include <lsp/effector/biquadratic_filter.hpp>

using namespace lsp;
using namespace lsp::midi::synth;

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
	require(wav.getNumChannels() == 1);

	if (preAmp < 0) {
		// MEMO 実効値の2乗=パワーを用いて正規化すると、それらしい音量で揃う(ラウドネスは考慮していないので注意)
		float rms = wav.getRMSLevel(0, 0, wav.getNumSamples());
		preAmp = mBaseRMS * mBaseRMS / (rms * rms + 1.0e-8f) * abs(preAmp);
	}
	mWaveTable.insert_or_assign(id, std::make_tuple(std::move(wav), preAmp, cycles));
}

generator::WaveTableGenerator<float> WaveTable::get(size_t id)const
{
	auto found = mWaveTable.find(id);
	if (found == mWaveTable.end()) {
		found = mWaveTable.find(Preset::Ground);
		check(found != mWaveTable.end());
	}

	auto& [wave, preAmp, cycles] = found->second;

	return generator::WaveTableGenerator<float>(wave, preAmp, cycles);
}

void WaveTable::reset()
{
	using FunctionGenerator = generator::FunctionGenerator<float>;
	using BiquadraticFilter = effector::BiquadraticFilter<float>;

	mNextCustomWaveId = CustomWaveIdBegin;
	mWaveTable.clear();

	// Ground
	{
		Signal<float> sig(1, 1);
		sig.setSample(0, 0, 0.f);
		add(Preset::Ground, std::move(sig), 0);
	}
	// SinWave
	{
		constexpr int samples = 512;
		Signal<float> sig(1, samples);
		FunctionGenerator fg;
		fg.setSinWave(samples, 1);
		for(int i = 0; i < samples; ++i) {
			sig.setSample(0, i, fg.update());
		}
		mBaseRMS = sig.getRMSLevel(0, 0, sig.getNumSamples());
		add(Preset::SinWave, std::move(sig), 1);
	}
	// SquareWave50
	{
		constexpr int samples = 512;
		Signal<float> sig(1, samples);
		FunctionGenerator fg;
		fg.setSquareWave(samples, 1);
		for (int i = 0; i < samples; ++i) {
			sig.setSample(0, i, fg.update());
		}
		add(Preset::SquareWave50, std::move(sig));
	}
	// SquareWave33
	{
		constexpr int samples = 512;
		Signal<float> sig(1, samples);
		FunctionGenerator fg;
		fg.setSquareWave(samples, 1, math::PI<float>/1.5f);
		for (int i = 0; i < samples; ++i) {
			sig.setSample(0, i, fg.update());
		}
		add(Preset::SquareWave33, std::move(sig));
	}
	// SquareWave25
	{
		constexpr int samples = 512;
		Signal<float> sig(1, samples);
		FunctionGenerator fg;
		fg.setSquareWave(samples, 1, math::PI<float>/2);
		for (int i = 0; i < samples; ++i) {
			sig.setSample(0, i, fg.update());
		}
		add(Preset::SquareWave25, std::move(sig));
	}
	// WhiteNoise
	{
		// MEMO サンプリング周波数よりサンプル数を大きくしないと、金属音のような規則性のある音が混ざることに注意
		constexpr int samples = 512;
		Signal<float> sig(1, samples);
		FunctionGenerator fg;
		fg.setWhiteNoise();
		for (int i = 0; i < samples; ++i) {
			sig.setSample(0, i, fg.update());
		}
		add(Preset::WhiteNoise, std::move(sig));
	}
	// DrumNoise : LSP用デフォルトドラム波形
	{
		constexpr int samples = 131072;
		Signal<float> sig(1, samples);
		FunctionGenerator fg;
		fg.setWhiteNoise();
		std::array<BiquadraticFilter, 5> bqfs;
		bqfs[0].setLopassParam(44100, 4000.f, 0.5f); // 不要高周波を緩やかにカットオフ
		bqfs[1].setLopassParam(44100, 4000.f, 0.5f); // (同上)
		bqfs[2].setLopassParam(44100, 3000.f, 0.5f); // (同上)
		bqfs[3].setLopassParam(44100, 2000.f, 0.5f); // (同上)
		bqfs[4].setLopassParam(44100, 1000.f, 1.0f); // 基本となる音程
		for(int i = 0; i < samples*2; i++) {		
			// 波形が安定するまで読み捨てる
			float s = fg.update();
			for(auto& bqf : bqfs) s = bqf.update(s);
		}
		for(int i = 0; i < samples; ++i) {
			float s = fg.update();
			for(auto& bqf : bqfs) s = bqf.update(s);
			sig.setSample(0, i, s);
		}
		add(Preset::DrumNoise, std::move(sig), -0.30f, 62.5f);
	}
}
