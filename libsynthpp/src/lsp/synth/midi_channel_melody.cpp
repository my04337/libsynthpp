#include <lsp/synth/midi_channel.hpp>
#include <lsp/synth/instruments.hpp>

using namespace lsp::synth;

std::unique_ptr<Voice> MidiChannel::createMelodyVoice(uint8_t noteNo, uint8_t vel)
{
	MelodyParam mp; // デフォルト値

	// 主要パラメータのロード (システム種別 + バンクセレクト付きフォールバック検索)
	if(auto found = mInstrumentTable.findMelodyParam(
		toInstrumentSystemType(mSystemType), ccBankSelectMSB, ccBankSelectLSB, mProgId)) {
		mp = *found;
	}

	float v = mp.volume;
	float a = mp.attack;
	float h = mp.hold;
	float d = mp.decay;
	float s = mp.sustain;
	float f = mp.fade;
	float r = mp.release;

	// 楽器毎の調整
	bool isDrumLikeInstrument = mp.isDrumLike;
	float noteNoAdjuster = mp.noteOffset;

	// 音色の反映
	auto wg = Instruments::createZeroWaveTable();
	if(isDrumLikeInstrument && mp.waveForm == MelodyWaveForm::Square) {
		// ドラム系 (波形指定なし) : ノイズジェネレータを使用
		wg = Instruments::createDrumNoiseGenerator();
	} else {
		switch(mp.waveForm) {
		case MelodyWaveForm::Sine:
			wg = Instruments::createSineGenerator();
			break;
		case MelodyWaveForm::Triangle:
			wg = Instruments::createTriangleGenerator();
			break;
		case MelodyWaveForm::Sawtooth:
			wg = Instruments::createSawtoothGenerator();
			break;
		case MelodyWaveForm::Noise:
			wg = Instruments::createDrumNoiseGenerator();
			break;
		case MelodyWaveForm::Square:
		default:
			wg = Instruments::createSquareGenerator();
			break;
		}
	}

	// CC 72/73/75 によるEGタイム調整（全システムタイプで適用）
	// 中心値64で等倍、0で約x0.006、127で約x190 の対数スケーリング
	float attackScale = calcEGTimeScale(ccAttackTime);
	float decayScale = calcEGTimeScale(ccDecayTime);
	float releaseScale = calcReleaseTimeScale();

	// NRPN (1, 99/100) によるアタック/ディケイタイム調整 (GS/XG)
	if(mSystemType.isGS() || mSystemType.isXG()) {
		attackScale *= calcEGTimeScale(getNRPN_MSB(1, 99).value_or(64));
		decayScale *= calcEGTimeScale(getNRPN_MSB(1, 100).value_or(64));
	}

	// ドラム風楽器はスケール係数を制限する (最大4倍)
	if(isDrumLikeInstrument) {
		attackScale = std::min(attackScale, 4.0f);
		decayScale = std::min(decayScale, 4.0f);
	}

	a *= attackScale;
	d *= decayScale;

	// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f) * v / ((s > 0.8f && s != 0.f) ? s : 0.8f);
	float thresholdLevel = 0.01f;  // ほぼ無音を長々再生するのを防ぐため、ほぼ聞き取れないレベルまで落ちたら止音する
	static const dsp::EnvelopeCurve<float> curveExp3(3.0f);

	if(isDrumLikeInstrument) {
		// ドラム風楽器 : DrumWaveTableVoice + DrumEnvelopeGenerator (AHD)
		auto voice = std::make_unique<DrumWaveTableVoice>(mSampleFreq, std::move(wg), noteNo, mCalculatedPitchBend, volume, ccPedal);
		voice->setNoteOffset(noteNoAdjuster);
		{
			float noteFreq = 440.f * exp2f((noteNo + noteNoAdjuster - 69.f) / 12.f);
			voice->setFilter(calcFilterCutoff(noteFreq), calcFilterQ());
		}
		voice->setVibrato(calcVibratoRate(), calcVibratoDepth(), calcVibratoDelay());

		auto& eg = voice->envelopeGenerator();
		eg.setEnvelope(
			static_cast<float>(mSampleFreq), curveExp3,
			std::max(0.005f, a),
			h,
			std::max(0.005f, d),
			thresholdLevel
		);
		eg.noteOn();
		return voice;
	} else {
		// 通常楽器 : MelodyWaveTableVoice + MelodyEnvelopeGenerator (AHDSFR)
		// ベースリリースタイムを保存（CC/NRPNスケーリング適用前）
		float baseReleaseTime = std::max(0.001f, r);
		r *= releaseScale;

		auto voice = std::make_unique<MelodyWaveTableVoice>(mSampleFreq, std::move(wg), noteNo, mCalculatedPitchBend, volume, ccPedal);
		voice->setNoteOffset(noteNoAdjuster);
		{
			float noteFreq = 440.f * exp2f((noteNo + noteNoAdjuster - 69.f) / 12.f);
			voice->setFilter(calcFilterCutoff(noteFreq), calcFilterQ());
		}
		voice->setBaseReleaseTime(baseReleaseTime);
		voice->setVibrato(calcVibratoRate(), calcVibratoDepth(), calcVibratoDelay());

		auto& eg = voice->envelopeGenerator();
		eg.setEnvelope(
			static_cast<float>(mSampleFreq), curveExp3,
			std::max(0.001f, a),
			h,
			std::max(0.001f, d),
			s,
			f,
			std::max(0.001f, r),
			thresholdLevel
		);
		eg.noteOn();
		return voice;
	}
}
