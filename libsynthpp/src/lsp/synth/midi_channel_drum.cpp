#include <lsp/synth/midi_channel.hpp>
#include <lsp/synth/instruments.hpp>

using namespace lsp::synth;

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
	{ 24, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.35f}}, //	
	{ 25, { 57, 1.50f, 0.05f, 0.00f, 1.00f, 0.50f}}, //
	{ 26, { 81, 2.00f, 0.00f, 0.00f, 0.20f, 0.50f}}, //
	{ 27, { 64, 2.00f, 0.00f, 0.00f, 0.20f, 0.50f}}, // High Q
	{ 28, { 69, 2.00f, 0.00f, 0.00f, 0.30f, 0.42f}}, // Slap
	{ 29, { 69, 1.80f, 0.00f, 0.00f, 0.30f, 0.50f}}, // Scratch Push
	{ 30, { 66, 1.80f, 0.00f, 0.00f, 0.40f, 0.50f}}, // Scratch Pull
	{ 31, { 69, 3.00f, 0.00f, 0.00f, 0.20f, 0.50f}}, // Sticks
	{ 32, { 69, 2.00f, 0.00f, 0.00f, 0.10f, 0.50f}}, // Square Click
	{ 33, { 66, 2.00f, 0.00f, 0.00f, 0.10f, 0.50f}}, // Metronome Click
	{ 34, { 78, 2.00f, 0.00f, 0.00f, 0.30f, 0.50f}}, // Metronome Bell
	{ 35, { 32, 4.59f, 0.00f, 0.00f, 0.20f, 0.50f}}, // Acoustic Bass Drum
	{ 36, { 32, 4.50f, 0.00f, 0.00f, 0.25f, 0.50f}}, // Bass Drum 1
	{ 37, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.50f}}, // Side Stick
	{ 38, { 44, 4.00f, 0.00f, 0.00f, 0.40f, 0.50f}}, // Snare 1
	{ 39, { 57, 2.00f, 0.00f, 0.05f, 0.40f, 0.42f}}, // Hand Clap
	{ 40, { 64, 3.00f, 0.00f, 0.00f, 0.40f, 0.50f}}, // Snare 2
	{ 41, { 44, 3.00f, 0.00f, 0.00f, 0.50f, 0.27f}}, // Low Floor Tom
	{ 42, { 64, 1.60f, 0.00f, 0.05f, 0.10f, 0.66f}}, // Closed Hi-hat
	{ 43, { 46, 3.00f, 0.00f, 0.00f, 0.50f, 0.36f}}, // High Floor Tom
	{ 44, { 66, 1.60f, 0.00f, 0.05f, 0.10f, 0.66f}}, // Pedal Hi-hat
	{ 45, { 48, 3.00f, 0.00f, 0.00f, 0.50f, 0.45f}}, // Low Tom
	{ 46, { 70, 1.60f, 0.00f, 0.10f, 0.75f, 0.66f}}, // Open Hi-hat
	{ 47, { 50, 3.00f, 0.00f, 0.00f, 0.40f, 0.55f}}, // Low-Mid Tom
	{ 48, { 53, 3.00f, 0.00f, 0.00f, 0.40f, 0.64f}}, // High-Mid Tom
	{ 49, { 68, 2.00f, 0.00f, 0.00f, 2.00f, 0.66f}}, // Crash Cymbal 1
	{ 50, { 56, 3.50f, 0.00f, 0.00f, 0.40f, 0.73f}}, // High Tom
	{ 51, { 66, 2.50f, 0.00f, 0.00f, 1.50f, 0.35f}}, // Ride Cymbal 1
	{ 52, { 62, 2.00f, 0.00f, 0.00f, 1.00f, 0.35f}}, // Chinese Cymbal
	{ 53, { 80, 2.00f, 0.00f, 0.00f, 1.00f, 0.35f}}, // Ride Bell
	{ 54, { 100,2.00f, 0.00f, 0.00f, 0.20f, 0.58f}}, // Tambourine
	{ 55, { 70, 1.60f, 0.00f, 0.00f, 1.50f, 0.42f}}, // Splash Cymbal
	{ 56, { 60, 3.00f, 0.00f, 0.00f, 0.40f, 0.66f}}, // Cowbell
	{ 57, { 72, 2.00f, 0.00f, 0.00f, 1.50f, 0.35f}}, // Crash Cymbal 2
	{ 58, { 60, 2.00f, 0.00f, 0.00f, 1.50f, 0.22f}}, // Vibra-slap
	{ 59, { 76, 1.20f, 0.00f, 0.00f, 1.50f, 0.35f}}, // Ride Cymbal 2
	{ 60, { 70, 2.00f, 0.00f, 0.00f, 0.20f, 0.77f}}, // High Bongo
	{ 61, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}}, // Low Bongo
	{ 62, { 70, 3.00f, 0.00f, 0.00f, 0.10f, 0.30f}}, // Mute Hi Conga
	{ 63, { 67, 2.00f, 0.00f, 0.00f, 0.30f, 0.30f}}, // Open Hi Conga
	{ 64, { 58, 2.00f, 0.00f, 0.00f, 0.35f, 0.35f}}, // Low Conga
	{ 65, { 70, 2.00f, 0.00f, 0.00f, 0.40f, 0.66f}}, // High Timbale
	{ 66, { 64, 2.00f, 0.00f, 0.00f, 0.40f, 0.66f}}, // Low Timbale
	{ 67, { 70, 2.00f, 0.00f, 0.00f, 0.20f, 0.22f}}, // High Agogo
	{ 68, { 64, 2.00f, 0.00f, 0.00f, 0.20f, 0.22f}}, // Low Agogo
	{ 69, { 70, 2.50f, 0.00f, 0.00f, 0.15f, 0.22f}}, // Cabasa
	{ 70, { 66, 2.00f, 0.00f, 0.00f, 0.20f, 0.19f}}, // Maracas
	{ 71, { 71, 2.00f, 0.00f, 0.10f, 0.10f, 0.77f}}, // Short Whistle
	{ 72, { 69, 2.00f, 0.00f, 0.30f, 0.10f, 0.77f}}, // Long Whistle
	{ 73, { 70, 2.50f, 0.00f, 0.00f, 0.15f, 0.73f}}, // Short Guiro
	{ 74, { 68, 1.50f, 0.00f, 0.30f, 0.10f, 0.73f}}, // Long Guiro
	{ 75, { 76, 2.00f, 0.00f, 0.00f, 0.20f, 0.66f}}, // Claves
	{ 76, { 70, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}}, // Hi Wood Block
	{ 77, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}}, // Low Wood Block
	{ 78, { 70, 2.00f, 0.00f, 0.00f, 0.40f, 0.35f}}, // Mute Cuica
	{ 79, { 64, 2.00f, 0.00f, 0.00f, 0.40f, 0.35f}}, // Open Cuica
	{ 80, { 86, 2.00f, 0.00f, 0.00f, 0.35f, 0.19f}}, // Mute Triangle
	{ 81, { 86, 0.70f, 0.00f, 0.10f, 0.20f, 0.19f}}, // Open Triangle
};

std::unique_ptr<Voice> MidiChannel::createDrumVoice(uint8_t noteNo, uint8_t vel)
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
		pan = panFromRPN.value_or(rand() % 128 - 64) / 127.f + 0.5f;
	}

	// NRPN : ドラムの音程微調整
	float resolvedNoteNo = static_cast<float>(pitch + getNRPN_MSB(24, noteNo).value_or(64) - 64);


	// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f);
	float cutoff_level = 0.01f;  // ほぼ無音を長々再生するのを防ぐため、ほぼ聞き取れないレベルまで落ちたらカットオフする
	static const dsp::EnvelopeGenerator<float>::Curve curveExp3(3.0f);

	float cutOffFreqRate = 2.f;
	float overtuneGain = 0.f; // dB
	if(mSystemType.isGS() || mSystemType.isXG()) {
		cutOffFreqRate = getNRPN_MSB(1, 32).value_or(64) / 128.f * 2.f + 1.f;
		overtuneGain = (getNRPN_MSB(1, 33).value_or(64) / 128.f - 0.5f) * 5.f;
	}

	auto wg = Instruments::createDrumNoiseGenerator();
	auto voice = std::make_unique<WaveTableVoice>(mSampleFreq, std::move(wg), resolvedNoteNo, mCalculatedPitchBend, volume, ccPedal);
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