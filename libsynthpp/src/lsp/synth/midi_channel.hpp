#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/system_type.hpp>
#include <lsp/midi/message.hpp>
#include <lsp/synth/instrument_table.hpp>
#include <lsp/synth/voice.hpp>
#include <array>

namespace lsp::synth
{
class WaveTable;

using StereoFrame = std::pair<float, float>;

class MidiChannel
	: non_copy
{
public:
	struct Digest {
		uint8_t ch = 0; // チャネル
		uint8_t progId = 0; // プログラムID
		uint8_t bankSelectMSB = 0; // バンクセレクト
		uint8_t bankSelectLSB = 0; // バンクセレクト
		float pan = 0.5f; // パン
		float volume = 1.0f; // チャネルボリューム
		float expression = 1.0f; // エクスプレッション
		float channelPressure = 1.0f; // チャネルプレッシャー
		float pitchBend = 0.0f; // ピッチベンド(ピッチベンドセンシティビティ考慮済み)
		size_t poly = 0; // 同時発音数
		uint8_t attackTime = 64; // アタックタイム
		uint8_t decayTime = 64; // ディケイタイム
		uint8_t releaseTime = 64; // リリースタイム
		uint8_t brightness = 64; // ブライトネス (カットオフ)
		uint8_t resonance = 64; // レゾナンス (ハーモニックコンテント)
		bool pedal = false; // ペダルOn/Off
		bool sostenuto = false; // ソステヌートOn/Off
		bool mono = false; // モノモードOn/Off
		bool drum = false; // ドラムか否か

		std::unordered_map<VoiceId, Voice::Digest> voices;
	};

	MidiChannel(uint32_t sampleFreq, uint8_t ch, const InstrumentTable& instrumentTable);

	void reset(midi::SystemType type);
	void resetVoices();
	void resetParameters();
	void resetParameterNumberState();
	// ---
	void noteOn(uint32_t noteNo, uint8_t vel);
	void noteOff(uint32_t noteNo);
	void noteCut(uint32_t noteNo);
	void programChange(uint8_t progId);
	void controlChange(uint8_t ctrlNo, uint8_t value);
	void channelPressure(uint8_t value);
	void polyphonicKeyPressure(uint8_t noteNo, uint8_t value);
	void pitchBend(int16_t pitch);
	void updateHold();
	void updateSostenuto();
	void setDrumMode(bool isDrumMode);
	// ---
	StereoFrame update();
	// ---
	Digest digest()const;
	// ---

private:
	// ボイスを生成します
	std::unique_ptr<Voice> createVoice(uint8_t noteNo, uint8_t vel);
	std::unique_ptr<Voice> createMelodyVoice(uint8_t noteNo, uint8_t vel);
	std::unique_ptr<Voice> createDrumVoice(uint8_t noteNo, uint8_t vel);

	void updatePitchBend();
	void updateReleaseTime();
	void updateFilter();

	// CC 72/73/75 および対応するNRPNからEGタイムスケーリング係数を計算します
	static float calcEGTimeScale(uint8_t ccValue);
	float calcReleaseTimeScale()const;

	// CC 71/74 および対応するNRPN(1,32/33)からフィルタパラメータを計算します
	float calcFilterCutoff(float noteFreq)const;
	float calcFilterQ()const;

	// RPN and NRPN
	std::optional<uint8_t> getRPN_MSB(uint8_t msb, uint8_t lsb)const noexcept;
	std::optional<uint8_t> getRPN_LSB(uint8_t msb, uint8_t lsb)const noexcept;

	std::optional<uint8_t> getNRPN_MSB(uint8_t msb, uint8_t lsb)const noexcept;
	std::optional<uint8_t> getNRPN_LSB(uint8_t msb, uint8_t lsb)const noexcept;
	
private:
	// サンプリング周波数(実行時に動的にセット)
	const uint32_t mSampleFreq;
	// チャネル番号(実行時に動的にセット)
	const uint8_t mMidiCh;
	// インストゥルメント情報テーブル
	const InstrumentTable& mInstrumentTable;

	// 発音中のボイス
	std::unordered_map<VoiceId, std::unique_ptr<Voice>> mVoices;

	// システムリセット種別
	midi::SystemType mSystemType;
		
	// プログラムチェンジ
	uint8_t mProgId; // プログラムId
	bool mIsDrumPart = false;

	// コントロールチェンジ
	uint8_t ccPrevCtrlNo;
	uint8_t ccPrevValue;
	uint8_t ccBankSelectMSB;// CC:0 - バンクセレクトMSB
	float ccVolume;			// CC:7 - チャネルボリューム
	float ccPan;			// CC:10 - パン [0.0(左), +1.0(右)]
	float ccExpression;		// CC:11 - エクスプレッション [0.0, +1.0]
	uint8_t ccBankSelectLSB;// CC:32 - バンクセレクトLSB
	bool	ccPedal;        // CC:64 - Hold1(ペダル)
	bool	ccSostenuto;    // CC:66 - ソステヌート
	uint8_t ccReleaseTime;	// CC:72 - リリースタイム
	uint8_t ccAttackTime;	// CC:73 - アタックタイム
	uint8_t ccDecayTime;	// CC:75 - ディケイタイム
	uint8_t ccResonance;	// CC:71 - レゾナンス (ハーモニックコンテント)
	uint8_t ccBrightness;	// CC:74 - ブライトネス (カットオフ)

	// チャネルモードメッセージ
	bool mMonoMode;         // CC:126/127 - モノ/ポリモード

	// チャネルプレッシャー  [0.0, 1.0]
	float mChannelPressure;

	// ピッチベンド
	int16_t mRawPitchBend;
	float mCalculatedPitchBend;

	// RPN/NRPN State
	std::optional<uint8_t> ccRPN_MSB;
	std::optional<uint8_t> ccRPN_LSB;
	std::optional<uint8_t> ccNRPN_MSB;
	std::optional<uint8_t> ccNRPN_LSB;
	std::optional<uint8_t> ccDE_MSB;
	std::optional<uint8_t> ccDE_LSB;

	std::unordered_map<uint16_t, std::pair<uint8_t, std::optional<int8_t>>> ccRPNs;
	std::unordered_map<uint16_t, std::pair<uint8_t, std::optional<int8_t>>> ccNRPNs;
};

}