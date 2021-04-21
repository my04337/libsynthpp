#include <LSP/Synth/MidiChannel.hpp>
#include <LSP/Synth/WaveTable.hpp>
#include <LSP/Synth/Voice.hpp>

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

	rpnNull = false;
	rpnPitchBendSensitibity = 2;

	resetParameterNumberState();
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

	// RPN/NRPNの受付が禁止されている場合、適用しない
	if (rpnNull) {
		apply_RPN_NRPN_state = false;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if (ccRPN_MSB == 0 && ccRPN_LSB == 0 && ccDE_MSB.has_value()) {
			// ピッチベンドセンシティビティ: MSBのみ使用
			rpnPitchBendSensitibity = ccDE_MSB.value();
			updatePitchBend();
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
				pan = 1.0f - (1.0f - pan) * ((1.0 - vpan) * 2);
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
MidiChannel::Info MidiChannel::info()const
{
	Info info;

	info.ch = mMidiCh;
	info.progId = mProgId;
	info.bankSelectMSB = ccBankSelectMSB;
	info.bankSelectLSB = ccBankSelectLSB;
	info.volume = ccVolume; 
	info.expression = ccExpression;
	info.pan = ccPan;
	info.pitchBend = mCalculatedPitchBend;
	info.attackTime = ccAttackTime;
	info.decayTime = ccDecayTime;
	info.releaseTime = ccReleaseTime;
	info.poly = mVoices.size();
	info.pedal = ccPedal;
	info.drum = mIsDrumPart;

	for (auto& kvp : mVoices) {
		info.voiceInfo.emplace(kvp.first, kvp.second->info());
	}

	return info;
}
std::unique_ptr<LSP::Synth::Voice> MidiChannel::createVoice(uint8_t noteNo, uint8_t vel)
{
	const float volume = (vel / 127.0f);
	static const LSP::Filter::EnvelopeGenerator<float>::Curve curveExp3(3.0f);
	Voice::EnvelopeGenerator eg;
	auto makeWaveTableVoice = [&](size_t waveTableId) {
		auto wg = mWaveTable.get(waveTableId);
		return std::make_unique<LSP::Synth::WaveTableVoice>(mSampleFreq, wg, eg, noteNo, mCalculatedPitchBend, volume, ccPedal);
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
		if (mProgId >= 0 && mProgId <= 7) {
			// ピアノ系
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -15.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if(mProgId >= 8 && mProgId <= 15) {
			// 音階付き打楽器系
			eg.setParam((float)mSampleFreq, curveExp3, 0.01f, 0.0f, 0.2f, 0.3f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 16 && mProgId <= 23) {
			// オルガン系
			eg.setParam((float)mSampleFreq, curveExp3, 0.03f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 24 && mProgId <= 28) {
			// ギター系 : 減衰早い
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -15.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 29 && mProgId <= 31) {
			// ギター系 : 減衰遅い
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 32 && mProgId <= 38) {
			// ベース系
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 40 && mProgId <= 47) {
			// ストリングス系
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 48 || mProgId >= 52 && mProgId <= 54) {
			// アンサンブル系 : 立ち上がり早い
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 49 && mProgId <= 51) {
			// アンサンブル系 : 立ち上がり遅い
			eg.setParam((float)mSampleFreq, curveExp3, 0.30f, 0.0f, 0.2f, 0.6f, -0.0f, 0.20f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 55) {
			// アンサンブル系 : オーケストラヒット
			eg.setParam((float)mSampleFreq, curveExp3, 0.10f, 0.0f, 0.2f, 0.4f, -30.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 64 && mProgId <= 71) {
			// ブラス系
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 72 && mProgId <= 79) {
			// リード系
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 72 && mProgId <= 79) {
			// 笛系
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 80 && mProgId <= 87) {
			// シンセサイザ系1
			eg.setParam((float)mSampleFreq, curveExp3, 0.03f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 88 || mProgId == 96 || mProgId == 98) {
			// シンセサイザ系2/3 : 打楽器寄り
			eg.setParam((float)mSampleFreq, curveExp3, 0.01f, 0.0f, 0.2f, 0.3f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 89 || mProgId == 92 || mProgId == 93 || mProgId == 95 || mProgId == 97 || mProgId == 101) {
			// シンセサイザ系2/3 : 立ち上がり遅い
			eg.setParam((float)mSampleFreq, curveExp3, 0.30f, 0.0f, 0.2f, 0.6f, -0.0f, 0.20f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 90 || mProgId == 94 || mProgId == 99 || mProgId == 100) {
			// シンセサイザ系2/3 : ベース系音色
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 91 || mProgId == 102 || mProgId == 103) {
			// シンセサイザ系2/3 : ストリングス系
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 104 && mProgId <= 105) {
			// 民族楽器 : ベース系 減衰遅い
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 106 && mProgId <= 108) {
			// 民族楽器 : ベース系 : 減衰早い
			eg.setParam((float)mSampleFreq, curveExp3, 0.02f, 0.0f, 0.2f, 0.4f, -20.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId >= 109 && mProgId <= 111) {
			// 民族楽器 : ストリングス, 笛系系
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.6f, -0.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 112 || mProgId == 114 || mProgId >= 116 && mProgId <= 118) {
			// 打楽器系 : 減衰早い
			eg.setParam((float)mSampleFreq, curveExp3, 0.01f, 0.0f, 0.2f, 0.3f, -10.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 113 || mProgId == 115) {
			// 打楽器系 : 減衰とても早い
			eg.setParam((float)mSampleFreq, curveExp3, 0.01f, 0.0f, 0.3f, 0.0f, -00.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		} else if (mProgId == 119) {
			// 打楽器系 : リバースシンバル
			eg.setParam((float)mSampleFreq, curveExp3, 1.2f, 0.0f, 0.1f, 0.0f, -00.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::WhiteNoise);
		} else if (mProgId == 120 || mProgId == 121 || mProgId == 127) {
			// 効果音系 : ノイズ, 減衰とても早い
			eg.setParam((float)mSampleFreq, curveExp3, 0.01f, 0.0f, 0.3f, 0.0f, -00.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::WhiteNoise);
		} else if (mProgId >= 122 && mProgId <= 126) {
			// 効果音系 : ノイズ とてもゆっくり
			eg.setParam((float)mSampleFreq, curveExp3, 1.2f, 0.0f, 0.5f, 0.6f, -0.0f, 0.50f);
			return makeWaveTableVoice(WaveTable::Preset::WhiteNoise);
		} else {
			eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		}
	} else {
		// TODO ドラム用音色を用意する
		eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f);
		return makeWaveTableVoice(WaveTable::Preset::Ground);
	}
}
void MidiChannel::updatePitchBend()
{
	mCalculatedPitchBend = rpnPitchBendSensitibity * (mRawPitchBend / 8192.0f);
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