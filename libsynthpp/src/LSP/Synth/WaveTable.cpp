#include <LSP/Synth/WaveTable.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>

using namespace LSP;
using namespace LSP::Synth;

WaveTable::WaveTable()
{
	reset();
}
size_t WaveTable::add(Signal<float>&& wav, float preAmp)
{
	size_t id = mNextCustomWaveId++;
	add(id, std::move(wav), preAmp);
	return id;
}
void WaveTable::add(size_t id, Signal<float>&& wav, float preAmp)
{
	lsp_assert(wav.channels() == 1);

	if (preAmp < 0) {
		// MEMO 実効値の2乗=パワーを用いて正規化すると、それらしい音量で揃う(ラウドネスは考慮していないので注意)
		float rms = calcRMS(wav);
		preAmp = mBaseRMS * mBaseRMS / (rms * rms + 1.0e-8f);
	}
	mWaveTable.insert_or_assign(id, std::make_tuple(std::move(wav), preAmp));
}

LSP::Generator::WaveTableGenerator<float> WaveTable::get(size_t id)const
{
	auto found = mWaveTable.find(id);
	if (found == mWaveTable.end()) {
		found = mWaveTable.find(Preset::Ground);
		lsp_assert(found != mWaveTable.end());
	}

	auto& [wave, preAmp] = found->second;

	return LSP::Generator::WaveTableGenerator<float>(wave, preAmp);
}

void WaveTable::reset()
{
	using FunctionGenerator = LSP::Generator::FunctionGenerator<float>;
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
	// BrownNoise
	{
		// MEMO サンプリング周波数よりサンプル数を大きくしないと、金属音のような規則性のある音が混ざることに注意
		constexpr size_t frames = 16384;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setBrownNoise();
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		add(Preset::BrownNoise, std::move(sig));
	}
}

float WaveTable::calcRMS(SignalView<float> wav)
{
	lsp_assert(wav.channels() == 1);

	// 信号の2乗平均値を取る
	const size_t frames = wav.frames();
	float p = 0;
	for (size_t i = 0; i < frames; ++i) {
		p += wav.frame(i)[0] * wav.frame(i)[0];
	}
	p /= frames;
	return std::sqrt(p);
}