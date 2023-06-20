#include <lsp/synth/midi_channel.hpp>

using namespace lsp;
using namespace lsp::midi;
using namespace lsp::synth;

static const std::unordered_map<
	uint8_t, std::tuple<
	uint8_t,// pitch : NoteNo
	float, // v: volume(adjuster)
	float, // a: sec
	float, // h: sec
	float, // d: sec
	float  // pan : 0～1
	>
> sDrumParams = {
	{ 24, { 64, 2.00, 0.00, 0.00, 0.30, 0.35f}},
	{ 25, { 57, 1.50, 0.05, 0.00, 1.00, 0.50f}},
	{ 26, { 81, 2.00, 0.00, 0.00, 0.20, 0.50f}},
	{ 27, { 64, 2.00, 0.00, 0.00, 0.20, 0.50f}},
	{ 28, { 69, 2.00, 0.00, 0.00, 0.30, 0.42f}},
	{ 29, { 69, 1.80, 0.00, 0.00, 0.30, 0.50f}},
	{ 30, { 66, 1.80, 0.00, 0.00, 0.40, 0.50f}},
	{ 31, { 69, 3.00, 0.00, 0.00, 0.20, 0.50f}},
	{ 32, { 69, 2.00, 0.00, 0.00, 0.10, 0.50f}},
	{ 33, { 66, 2.00, 0.00, 0.00, 0.10, 0.50f}},
	{ 34, { 78, 2.00, 0.00, 0.00, 0.30, 0.50f}},
	{ 35, { 32, 4.59, 0.00, 0.00, 0.20, 0.50f}},
	{ 36, { 32, 4.50, 0.00, 0.00, 0.25, 0.50f}},
	{ 37, { 64, 2.00, 0.00, 0.00, 0.30, 0.50f}},
	{ 38, { 44, 4.00, 0.00, 0.00, 0.40, 0.50f}},
	{ 39, { 57, 2.00, 0.00, 0.05, 0.40, 0.42f}},
	{ 40, { 64, 3.00, 0.00, 0.00, 0.40, 0.50f}},
	{ 41, { 44, 3.00, 0.00, 0.00, 0.50, 0.27f}},
	{ 42, { 64, 1.60, 0.00, 0.05, 0.10, 0.66f}},
	{ 43, { 46, 3.00, 0.00, 0.00, 0.50, 0.36f}},
	{ 44, { 66, 1.60, 0.00, 0.05, 0.10, 0.66f}},
	{ 45, { 48, 3.00, 0.00, 0.00, 0.50, 0.45f}},
	{ 46, { 70, 1.60, 0.00, 0.10, 0.75, 0.66f}},
	{ 47, { 50, 3.00, 0.00, 0.00, 0.40, 0.55f}},
	{ 48, { 53, 3.00, 0.00, 0.00, 0.40, 0.64f}},
	{ 49, { 68, 2.00, 0.00, 0.00, 2.00, 0.66f}},
	{ 50, { 56, 3.50, 0.00, 0.00, 0.40, 0.73f}},
	{ 51, { 66, 2.50, 0.00, 0.00, 1.50, 0.35f}},
	{ 52, { 62, 2.00, 0.00, 0.00, 1.00, 0.35f}},
	{ 53, { 80, 2.00, 0.00, 0.00, 1.00, 0.35f}},
	{ 54, { 100,2.00, 0.00, 0.00, 0.20, 0.58f}},
	{ 55, { 70, 1.60, 0.00, 0.00, 1.50, 0.42f}},
	{ 56, { 60, 3.00, 0.00, 0.00, 0.40, 0.66f}},
	{ 57, { 72, 2.00, 0.00, 0.00, 1.50, 0.35f}},
	{ 58, { 60, 2.00, 0.00, 0.00, 1.50, 0.22f}},
	{ 59, { 76, 1.20, 0.00, 0.00, 1.50, 0.35f}},
	{ 60, { 70, 2.00, 0.00, 0.00, 0.20, 0.77f}},
	{ 61, { 64, 2.00, 0.00, 0.00, 0.30, 0.77f}},
	{ 62, { 70, 3.00, 0.00, 0.00, 0.10, 0.30f}},
	{ 63, { 67, 2.00, 0.00, 0.00, 0.30, 0.30f}},
	{ 64, { 58, 2.00, 0.00, 0.00, 0.35, 0.35f}},
	{ 65, { 70, 2.00, 0.00, 0.00, 0.40, 0.66f}},
	{ 66, { 64, 2.00, 0.00, 0.00, 0.40, 0.66f}},
	{ 67, { 70, 2.00, 0.00, 0.00, 0.20, 0.22f}},
	{ 68, { 64, 2.00, 0.00, 0.00, 0.20, 0.22f}},
	{ 69, { 70, 2.50, 0.00, 0.00, 0.15, 0.22f}},
	{ 70, { 66, 2.00, 0.00, 0.00, 0.20, 0.19f}},
	{ 71, { 71, 2.00, 0.00, 0.10, 0.10, 0.77f}},
	{ 72, { 69, 2.00, 0.00, 0.30, 0.10, 0.77f}},
	{ 73, { 70, 2.50, 0.00, 0.00, 0.15, 0.73f}},
	{ 74, { 68, 1.50, 0.00, 0.30, 0.10, 0.73f}},
	{ 75, { 76, 2.00, 0.00, 0.00, 0.20, 0.66f}},
	{ 76, { 70, 2.00, 0.00, 0.00, 0.30, 0.77f}},
	{ 77, { 64, 2.00, 0.00, 0.00, 0.30, 0.77f}},
	{ 78, { 70, 2.00, 0.00, 0.00, 0.40, 0.35f}},
	{ 79, { 64, 2.00, 0.00, 0.00, 0.40, 0.35f}},
	{ 80, { 86, 2.00, 0.00, 0.00, 0.35, 0.19f}},
	{ 81, { 86, 0.70, 0.00, 0.10, 0.20, 0.19f}},
};

std::unique_ptr<lsp::synth::Voice> lsp::synth::MidiChannel::createDrumVoice(uint8_t noteNo, uint8_t vel)
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
	if(auto panFromRPN = getNRPN_MSB(28, noteNo)) {
		pan = panFromRPN.value_or(rand() % 128) / 127.f;
	}

	// NRPN : ドラムの音程微調整
	float resolvedNoteNo = static_cast<float>(pitch + getNRPN_MSB(24, noteNo).value_or(64) - 64);


	// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f);
	float cutoff_level = 0.01f;
	static const lsp::effector::EnvelopeGenerator<float>::Curve curveExp3(3.0f);

	float cutOffFreqRate = 2.f;
	float overtuneGain = 0.f; // dB
	if(mSystemType != SystemType::GM1) {
		cutOffFreqRate = getNRPN_MSB(1, 32).value_or(64) / 128.f * 2.f + 1.f;
		overtuneGain = (getNRPN_MSB(1, 33).value_or(64) / 128.f - 0.5f) * 5.f;
	}

	auto wg = mWaveTable.get(WaveTable::Preset::DrumNoise);
	auto voice = std::make_unique<lsp::synth::WaveTableVoice>(mSampleFreq, wg, resolvedNoteNo, mCalculatedPitchBend, volume, ccPedal);
	voice->setPan(pan);
	voice->setResonance(cutOffFreqRate, overtuneGain);

	auto& eg = voice->envolopeGenerator();
	eg.setDrumEnvelope(
		mSampleFreq, curveExp3,
		std::max(0.005f, a),
		h,
		std::max(0.005f, d),
		cutoff_level
	);
	eg.noteOn();

	return voice;
}