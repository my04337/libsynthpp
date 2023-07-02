/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/midi/synth/midi_channel.hpp>
#include <lsp/midi/synth/luath_synth.hpp>

using namespace lsp::midi::synth;

static const std::unordered_map<
	int, std::tuple<
	int,// pitch : NoteNo
	float, // v: volume(adjuster)
	float, // a: sec
	float, // h: sec
	float, // d: sec
	float  // pan : 0～1
	>
> sDrumParams = {
	{ 24, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.35f}},
	{ 25, { 57, 1.50f, 0.05f, 0.00f, 1.00f, 0.50f}},
	{ 26, { 81, 2.00f, 0.00f, 0.00f, 0.20f, 0.50f}},
	{ 27, { 64, 2.00f, 0.00f, 0.00f, 0.20f, 0.50f}},
	{ 28, { 69, 2.00f, 0.00f, 0.00f, 0.30f, 0.42f}},
	{ 29, { 69, 1.80f, 0.00f, 0.00f, 0.30f, 0.50f}},
	{ 30, { 66, 1.80f, 0.00f, 0.00f, 0.40f, 0.50f}},
	{ 31, { 69, 3.00f, 0.00f, 0.00f, 0.20f, 0.50f}},
	{ 32, { 69, 2.00f, 0.00f, 0.00f, 0.10f, 0.50f}},
	{ 33, { 66, 2.00f, 0.00f, 0.00f, 0.10f, 0.50f}},
	{ 34, { 78, 2.00f, 0.00f, 0.00f, 0.30f, 0.50f}},
	{ 35, { 32, 4.59f, 0.00f, 0.00f, 0.20f, 0.50f}},
	{ 36, { 32, 4.50f, 0.00f, 0.00f, 0.25f, 0.50f}},
	{ 37, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.50f}},
	{ 38, { 44, 4.00f, 0.00f, 0.00f, 0.40f, 0.50f}},
	{ 39, { 57, 2.00f, 0.00f, 0.05f, 0.40f, 0.42f}},
	{ 40, { 64, 3.00f, 0.00f, 0.00f, 0.40f, 0.50f}},
	{ 41, { 44, 3.00f, 0.00f, 0.00f, 0.50f, 0.27f}},
	{ 42, { 64, 1.60f, 0.00f, 0.05f, 0.10f, 0.66f}},
	{ 43, { 46, 3.00f, 0.00f, 0.00f, 0.50f, 0.36f}},
	{ 44, { 66, 1.60f, 0.00f, 0.05f, 0.10f, 0.66f}},
	{ 45, { 48, 3.00f, 0.00f, 0.00f, 0.50f, 0.45f}},
	{ 46, { 70, 1.60f, 0.00f, 0.10f, 0.75f, 0.66f}},
	{ 47, { 50, 3.00f, 0.00f, 0.00f, 0.40f, 0.55f}},
	{ 48, { 53, 3.00f, 0.00f, 0.00f, 0.40f, 0.64f}},
	{ 49, { 68, 2.00f, 0.00f, 0.00f, 2.00f, 0.66f}},
	{ 50, { 56, 3.50f, 0.00f, 0.00f, 0.40f, 0.73f}},
	{ 51, { 66, 2.50f, 0.00f, 0.00f, 1.50f, 0.35f}},
	{ 52, { 62, 2.00f, 0.00f, 0.00f, 1.00f, 0.35f}},
	{ 53, { 80, 2.00f, 0.00f, 0.00f, 1.00f, 0.35f}},
	{ 54, { 100,2.00f, 0.00f, 0.00f, 0.20f, 0.58f}},
	{ 55, { 70, 1.60f, 0.00f, 0.00f, 1.50f, 0.42f}},
	{ 56, { 60, 3.00f, 0.00f, 0.00f, 0.40f, 0.66f}},
	{ 57, { 72, 2.00f, 0.00f, 0.00f, 1.50f, 0.35f}},
	{ 58, { 60, 2.00f, 0.00f, 0.00f, 1.50f, 0.22f}},
	{ 59, { 76, 1.20f, 0.00f, 0.00f, 1.50f, 0.35f}},
	{ 60, { 70, 2.00f, 0.00f, 0.00f, 0.20f, 0.77f}},
	{ 61, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}},
	{ 62, { 70, 3.00f, 0.00f, 0.00f, 0.10f, 0.30f}},
	{ 63, { 67, 2.00f, 0.00f, 0.00f, 0.30f, 0.30f}},
	{ 64, { 58, 2.00f, 0.00f, 0.00f, 0.35f, 0.35f}},
	{ 65, { 70, 2.00f, 0.00f, 0.00f, 0.40f, 0.66f}},
	{ 66, { 64, 2.00f, 0.00f, 0.00f, 0.40f, 0.66f}},
	{ 67, { 70, 2.00f, 0.00f, 0.00f, 0.20f, 0.22f}},
	{ 68, { 64, 2.00f, 0.00f, 0.00f, 0.20f, 0.22f}},
	{ 69, { 70, 2.50f, 0.00f, 0.00f, 0.15f, 0.22f}},
	{ 70, { 66, 2.00f, 0.00f, 0.00f, 0.20f, 0.19f}},
	{ 71, { 71, 2.00f, 0.00f, 0.10f, 0.10f, 0.77f}},
	{ 72, { 69, 2.00f, 0.00f, 0.30f, 0.10f, 0.77f}},
	{ 73, { 70, 2.50f, 0.00f, 0.00f, 0.15f, 0.73f}},
	{ 74, { 68, 1.50f, 0.00f, 0.30f, 0.10f, 0.73f}},
	{ 75, { 76, 2.00f, 0.00f, 0.00f, 0.20f, 0.66f}},
	{ 76, { 70, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}},
	{ 77, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}},
	{ 78, { 70, 2.00f, 0.00f, 0.00f, 0.40f, 0.35f}},
	{ 79, { 64, 2.00f, 0.00f, 0.00f, 0.40f, 0.35f}},
	{ 80, { 86, 2.00f, 0.00f, 0.00f, 0.35f, 0.19f}},
	{ 81, { 86, 0.70f, 0.00f, 0.10f, 0.20f, 0.19f}},
};

std::unique_ptr<Voice> MidiChannel::createDrumVoice(int noteNo, float vel)
{
	uint8_t pitch = 69;
	float v = 1.f; // volume(adjuster)
	float a = 0.f; // sec
	float h = 0.f; // sec
	float d = 0.f; // sec
	float pan = 0.5;

	// 主要パラメータのロード
	if(auto found = sDrumParams.find(noteNo); found != sDrumParams.end()) {
		std::tie(pitch, v, a, h, d, pan) = found->second;
	}

	// NRPN : パンが指定されている場合、オーバーライドする
	if(auto panFromRPN = getInt7NRPN(28, noteNo)) {
		pan = panFromRPN.value_or(rand() % 128 - 64) / 127.f + 0.5f;
	}

	// NRPN : ドラムの音程微調整
	float resolvedNoteNo = static_cast<float>(pitch + getInt7NRPN(24, noteNo).value_or(0));


	// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	float volume = powf(10.f, -20.f * (1.f - vel) / 20.f);
	float cutoff_level = 0.01f;
	static const effector::EnvelopeGenerator<float>::Curve curveExp3(3.0f);

	float cutOffFreqRate = 2.f;
	float overtuneGain = 0.f; // dB
	if(mSystemType.isGS() || mSystemType.isXG()) {
		cutOffFreqRate = (getInt7NRPN(1, 32).value_or(0) + 64) / 128.f * 2.f + 1.f;
		overtuneGain = getInt7NRPN(1, 33).value_or(0) / 128.f * 5.f;
	}

	auto wg = mSynth.presetWaveTable().createWaveGenerator(WaveTable::Preset::DrumNoise);
	auto voice = std::make_unique<WaveTableVoice>(mSynth.sampleFreq(), wg, resolvedNoteNo, mCalculatedPitchBend, volume, ccPedal);
	voice->setPan(pan);
	voice->setResonance(cutOffFreqRate, overtuneGain);

	auto& eg = voice->envolopeGenerator();
	eg.setDrumEnvelope(mSynth.sampleFreq(), curveExp3, a, h, d, cutoff_level);
	eg.noteOn();

	return voice;
}