#include <LSP/Synth/WaveTable.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>

using namespace LSP;
using namespace LSP::Synth;

WaveTable::WaveTable()
{
	reset();
}
size_t WaveTable::add(Signal<float>&& wav)
{
	size_t id = mNextCustomWaveId++;
	add(id, std::move(wav));
	return id;
}
void WaveTable::add(size_t id, Signal<float>&& wav)
{
	mWaveTable.insert_or_assign(id, std::move(wav));
}

SignalView<float> WaveTable::get(size_t id)const
{
	auto found = mWaveTable.find(id);
	if (found == mWaveTable.end()) {
		return mWaveTable.at(Preset::Ground);
	}

	return found->second;
}

void WaveTable::reset()
{
	using FunctionGenerator = LSP::Generator::FunctionGenerator<float>;
	// Ground
	{
		auto sig = Signal<float>::allocate(1);
		*sig.data() = 0;
		add(Preset::Ground, std::move(sig));
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
		add(Preset::SinWave, std::move(sig));
	}
	// SquareWave
	{
		constexpr size_t frames = 512;
		auto sig = Signal<float>::allocate(frames);
		FunctionGenerator fg;
		fg.setSquareWave(frames, 1);
		for (size_t i = 0; i < frames; ++i) {
			sig.frame(i)[0] = fg.update();
		}
		add(Preset::SquareWave, std::move(sig));
	}
}