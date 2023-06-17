#include <LSP/Synth/MidiChannel.hpp>
#include <LSP/Synth/WaveTable.hpp>
#include <LSP/Synth/Voice.hpp>
#include <LSP/Synth/Instrument.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::Synth;

MidiChannel::MidiChannel(uint32_t sampleFreq, uint8_t ch, const WaveTable& waveTable)
	: mSampleFreq(sampleFreq)
	, mMidiCh(ch)
	, mWaveTable(waveTable)
{
	reset(LSP::MIDI::SystemType::GM1);
}
// チャネル毎パラメータ類 リセット
void MidiChannel::reset(SystemType type)
{
	mSystemType = type;

	resetVoices();
	resetParameters();
}
void MidiChannel::resetVoices()
{
	// 全発音を強制停止
	mVoices.clear();
}
void MidiChannel::resetParameters()
{
	// チャネル ボイス メッセージ系
	mProgId = 0; // Acoustic Piano
	mIsDrumPart = (mMidiCh == 9);
	mRawPitchBend = 0;
	mCalculatedPitchBend = 0;

	// コントロールチェンジ系
	ccVolume = 1.0;
	ccPan = 0.5f;
	ccExpression = 1.0;
	ccPedal = false;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	ccBankSelectLSB = 0;
	ccBankSelectMSB = 0;

	ccAttackTime = 64;
	ccDecayTime = 64;
	ccReleaseTime = 64;

	resetParameterNumberState();

	ccRPNs.clear();
	ccNRPNs.clear();
}

void MidiChannel::resetParameterNumberState()
{
	ccRPN_MSB.reset();
	ccRPN_LSB.reset();
	ccNRPN_MSB.reset();
	ccNRPN_LSB.reset();
	ccDE_MSB.reset();
	ccDE_LSB.reset();
}
void MidiChannel::noteOn(uint32_t noteNo, uint8_t vel)
{
	// 同じノート番号は同時発音不可
	noteOff(noteNo);
	// ---
	if (vel > 0) {
		auto id = VoiceId::issue();
		auto voice = createVoice(noteNo, vel);
		mVoices.emplace(id, std::move(voice));
	}
}
void MidiChannel::noteOff(uint32_t noteNo)
{
	for (auto& [id, voice] : mVoices) {
		if (voice->noteNo() == noteNo) {
			voice->noteOff();
		}
	}
}
// プログラムチェンジ
void MidiChannel::programChange(uint8_t progId)
{
	// 事前に受信していたバンクセレクトを解決
	// プログラムId更新
	mProgId = progId;
}
// コントロールチェンジ & チャネルモードメッセージ
void MidiChannel::controlChange(uint8_t ctrlNo, uint8_t value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html
	//        https://www.g200kg.com/jp/docs/tech/midi.html

	bool apply_RPN_NRPN_state = false;

	switch (ctrlNo) {
	// --- コントロールチェンジ ---
	case 0: // Bank Select <MSB>（バンクセレクト）
		ccBankSelectMSB = value;
		break;
	case 6: // Data Entry(MSB)
		ccDE_MSB = value;
		ccDE_LSB.reset();
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 10: // Pan(パン)
		// MEMO 中央値は64。 1-127の範囲を取る実装が多い
		ccPan = std::clamp((value - 1) / 126.0f, 0.0f, 1.0f);
		break;
	case 7: // Channel Volumeチャンネルボリューム）
		ccVolume = (value / 127.0f);
		break;
	case 11: // Expression(エクスプレッション)
		ccExpression = (value / 127.0f);
		break;
	case 32: // Bank Select <LSB>（バンクセレクト）
		ccBankSelectLSB = value;
		break;
	case 36: // Data Entry(LSB)
		ccDE_LSB = value;
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 64: // Hold1(ホールド1:ダンパーペダル)
		ccPedal = (value >= 0x64);
		updateHold();
		break;
	case 72: // Release Time(リリースタイム)
		ccReleaseTime = value;
		break;
	case 73: // Attack Time(アタックタイム)
		ccAttackTime = value;
		break;
	case 75: // Decay Time(ディケイタイム)
		ccDecayTime = value;
		break;
	case 98: // NRPN(LSB)
		ccNRPN_LSB = value;
		break;
	case 99: // NRPN(MSB)
		resetParameterNumberState();
		ccNRPN_MSB = value;
		break;
	case 100: // RPN(LSB)
		ccRPN_LSB = value;
		apply_RPN_NRPN_state = true; // RPNヌルはこのタイミングで適用する
		break;
	case 101: // RPN(MSB)
		resetParameterNumberState();
		ccRPN_MSB = value;
		break;

	// --- チャネルモードメッセージ ---
	case 120: // オールサウンドオフ
		resetVoices();
		break;
	case 121: // リセットオールコントローラ
		resetParameters();
		break;
	case 123: // オールノートオフ
		for (auto& kvp : mVoices) {
			kvp.second->noteOff(true);
		}
		break;
	// --- チャネルモードメッセージ : not implemented ---
	case 122: // ローカルコントロール
	case 124: // オムニオフ
	case 125: // オムニオン
	case 126: // モノモード
	case 127: // モノモード
		break;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if(ccRPN_MSB == 0x7F && ccRPN_MSB == 0x7F) {
			// RPNヌル
			resetParameterNumberState();
		}
		else if(ccDE_MSB || ccDE_LSB) {
			if(ccRPN_MSB && ccRPN_LSB) {
				auto key = (static_cast<uint16_t>(*ccRPN_MSB) << 8) | *ccRPN_LSB;
				if(ccDE_LSB) {
					ccRPNs[key] = (0xFF00 & ccRPNs[key]) | static_cast<uint16_t>(*ccDE_MSB);
				}
				else {
					ccRPNs[key] = static_cast<uint16_t>(*ccDE_MSB) << 8;
				}
			}
			if(ccNRPN_MSB && ccNRPN_LSB) {
				auto key = (static_cast<uint16_t>(*ccNRPN_MSB) << 8) | *ccNRPN_LSB;
				if(ccDE_LSB) {
					ccNRPNs[key] = (0xFF00 & ccNRPNs[key]) | static_cast<uint16_t>(*ccDE_MSB);
				}
				else {
					ccNRPNs[key] = static_cast<uint16_t>(*ccDE_MSB) << 8;
				}
			}

			// RPN(即時反映): ピッチベンドセンシティビティ
			if(ccRPN_MSB == 0 && ccRPN_LSB == 0 && ccDE_MSB) {
				updatePitchBend();
			}
		}
	}

	ccPrevCtrlNo = ctrlNo;
	ccPrevValue = value;
}

void MidiChannel::pitchBend(int16_t pitch)
{
	mRawPitchBend = pitch;
	updatePitchBend();
}

StereoFrame MidiChannel::update()
{
	// オシレータからの出力はモノラル
	StereoFrame ret = std::make_pair(0.0f, 0.0f);
	for (auto iter = mVoices.begin(); iter != mVoices.end();) {
		auto& voice = *iter->second;
		// ボイス単体の音を生成
		float v = voice.update();

		// パン適用
		float pan = ccPan;
		if (voice.pan().has_value()) {
			float vpan = *voice.pan();
			if (vpan < 0.5f) {
				// vpan=0 : 左, vpan=0.5 : 元のpan
				pan = pan * (vpan * 2);
			} else {
				// vpan=0.5 : 元のpan, vpan=1.0 : 右
				pan = 1.0f - (1.0f - pan) * ((1.0f - vpan) * 2);
			}
		}

		ret.first += v * (1.0f - pan); // L ch
		ret.second += v * pan;         // R ch


		// 発音終了済のボイスを破棄
		if (iter->second->envolopeGenerator().isBusy()) {
			++iter;
		} else {
			iter = mVoices.erase(iter);
		}
	}

	// ボリューム
	ret.first *= ccVolume;
	ret.second *= ccVolume;

	// エクスプレッション
	ret.first *= ccExpression;
	ret.second *= ccExpression;

	return ret;
}
MidiChannel::Digest MidiChannel::digest()const
{
	Digest digest;

	digest.ch = mMidiCh;
	digest.progId = mProgId;
	digest.bankSelectMSB = ccBankSelectMSB;
	digest.bankSelectLSB = ccBankSelectLSB;
	digest.volume = ccVolume; 
	digest.expression = ccExpression;
	digest.pan = ccPan;
	digest.pitchBend = mCalculatedPitchBend;
	digest.attackTime = ccAttackTime;
	digest.decayTime = ccDecayTime;
	digest.releaseTime = ccReleaseTime;
	digest.poly = mVoices.size();
	digest.pedal = ccPedal;
	digest.drum = mIsDrumPart;

	for (auto& [id, voice] : mVoices) {
		auto& eg = voice->envolopeGenerator();
		digest.voices.emplace(id, voice->digest());
	}

	return digest;
}
std::unique_ptr<LSP::Synth::Voice> MidiChannel::createVoice(uint8_t noteNo, uint8_t vel)
{
	static const LSP::Filter::EnvelopeGenerator<float>::Curve curveExp3(3.0f);
	Voice::EnvelopeGenerator eg;

	auto adjustADR = [&](float& attack_time, float& decay_time, float& release_time) {
		if(mSystemType != SystemType::GM1) {
			attack_time  *= powf(10.0f, (ccAttackTime  / 127.f - 0.5f) * 4.556f);
			decay_time   *= powf(10.0f, (ccDecayTime   / 127.f - 0.5f) * 4.556f);
			release_time *= powf(10.0f, (ccReleaseTime / 127.f - 0.5f) * 4.556f);
		}
	};

	/*
	* MEMO : EnvelopeGenerator::setParam(AHDSFR系)の引数
	*	attack_time,	// sec
	*	hold_time,		// sec
	*	decay_time,		// sec
	*	sustain_level,	// level
	*	fade_slope,		// Linear : level/sec, Exp : dBFS/sec
	*	release_time	// sec)
	*/
	

	if (!mIsDrumPart) {
		// 参考 : http://www2u.biglobe.ne.jp/~rachi/midinst.htm
		auto [preamp, attack_time, hold_time, decay_time, sustain_level, release_time] 
			= LSP::Synth::Instrument::getDefaultMelodyEnvelopeParams(mProgId, 0);
		adjustADR(attack_time, decay_time, release_time);

		// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
		// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
		float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f) * preamp / ((sustain_level > 0.8f && sustain_level != 0.f) ? sustain_level : 0.8f);

		eg.setParam((float)mSampleFreq, curveExp3, attack_time, hold_time, decay_time, sustain_level, 0.f, release_time);
		auto wg = mWaveTable.get(WaveTable::Preset::SquareWave);
		auto voice = std::make_unique<LSP::Synth::WaveTableVoice>(mSampleFreq, wg, eg, noteNo, mCalculatedPitchBend, volume, ccPedal);
		return voice;
	} else {
		// TODO ドラム用音色を用意する
		
		// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
		// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
		float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f);

		eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f);
		auto wg = mWaveTable.get(WaveTable::Preset::WhiteNoise);
		auto voice = std::make_unique<LSP::Synth::WaveTableVoice>(mSampleFreq, wg, eg, noteNo, mCalculatedPitchBend, volume, ccPedal);
		voice->setPan(LSP::Synth::Instrument::getDefaultDrumPan(noteNo));
		return voice;
	}
}
void MidiChannel::updatePitchBend()
{
	mCalculatedPitchBend = rpnPitchBendSensitibity() * (mRawPitchBend / 8192.0f);
	for (auto& kvp : mVoices) {
		kvp.second->setPitchBend(mCalculatedPitchBend);
	}
}
void MidiChannel::updateHold()
{
	for (auto& [id, voice] : mVoices) {
		voice->setHold(ccPedal);
	}

}
uint8_t MidiChannel::rpnPitchBendSensitibity()const noexcept
{
	auto found = ccRPNs.find(0x0000);
	return found != ccRPNs.end() ? static_cast<uint8_t>(found->second >> 8) : /*デフォルト ピッチベンド*/2; 
}