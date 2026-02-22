#include <lsp/synth/midi_channel.hpp>
#include <lsp/synth/instruments.hpp>

using namespace lsp::synth;

std::unique_ptr<Voice> MidiChannel::createDrumVoice(uint8_t noteNo, uint8_t vel)
{
	DrumParam dp; // デフォルト値

	// 主要パラメータのロード (システム種別 + バンクセレクト付きフォールバック検索)
	if(auto found = mInstrumentTable.findDrumParam(
		toInstrumentSystemType(mSystemType), ccBankSelectMSB, ccBankSelectLSB, noteNo)) {
		dp = *found;
	}

	uint8_t pitch = static_cast<uint8_t>(dp.pitch);
	float v = dp.volume;
	float a = dp.attack;
	float h = dp.hold;
	float d = dp.decay;
	float pan = dp.pan;

	// NRPN (28, noteNo) : ドラムパン
	// value 0 = ランダム、1 = 左端、64 = 中央、127 = 右端
	if(auto panFromNRPN = getNRPN_MSB(28, noteNo)) {
		uint8_t panValue = *panFromNRPN;
		if(panValue == 0) {
			pan = static_cast<float>(rand() % 127 + 1) / 127.f;
		} else {
			pan = std::clamp((panValue - 1) / 126.0f, 0.0f, 1.0f);
		}
	}

	// NRPN (24, noteNo) : ドラムピッチ粗調整 (中心値64 = 変化なし、±半音単位)
	// NRPN (25, noteNo) : ドラムピッチ微調整 (中心値64 = 変化なし、±1半音の範囲)
	float coarseOffset = static_cast<float>(getNRPN_MSB(24, noteNo).value_or(64) - 64);
	float fineOffset = (getNRPN_MSB(25, noteNo).value_or(64) - 64) / 64.0f;
	float resolvedNoteNo = static_cast<float>(pitch) + coarseOffset + fineOffset;

	// NRPN (26, noteNo) : ドラムレベル (0-127、デフォルト127相当)
	float drumLevelScale = 1.0f;
	if(auto drumLevel = getNRPN_MSB(26, noteNo)) {
		drumLevelScale = *drumLevel / 127.0f;
	}

	// CC 73/75 によるEGタイム調整（全システムタイプで適用）
	// ドラムは元々長いDecayを持つ楽器があるため、スケール係数を制限する (最大4倍)
	float attackScale = std::min(calcEGTimeScale(ccAttackTime), 4.0f);
	float decayScale = std::min(calcEGTimeScale(ccDecayTime), 4.0f);
	a *= attackScale;
	d *= decayScale;

	// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f) * v * drumLevelScale;
	float threshold_level = 0.01f;  // ほぼ無音を長々再生するのを防ぐため、ほぼ聞き取れないレベルまで落ちたら止音する
	static const dsp::EnvelopeCurve<float> curveExp3(3.0f);

	auto wg = Instruments::createDrumNoiseGenerator();
	auto voice = std::make_unique<DrumWaveTableVoice>(mSampleFreq, std::move(wg), noteNo, mCalculatedPitchBend, volume, ccPedal);
	voice->setNoteOffset(resolvedNoteNo - static_cast<float>(noteNo));
	voice->setPan(pan);
	{
		float noteFreq = 440.f * exp2f((resolvedNoteNo - 69.f) / 12.f);
		voice->setFilter(calcFilterCutoff(noteFreq), calcFilterQ());
	}

	auto& eg = voice->envelopeGenerator();
	eg.setEnvelope(
		static_cast<float>(mSampleFreq), curveExp3,
		std::max(0.005f, a),
		h,
		std::max(0.005f, d),
		threshold_level
	);
	eg.noteOn();

	return voice;
}