#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>
#include <LSP/Synth/VoiceMapper.hpp>
#include <LSP/Synth/Voice.hpp>

#include <array>

namespace LSP::Synth
{
class WaveTable;

class MidiChannel
{
public:
	struct Info {
		uint8_t ch = 0; // チャネル
		uint8_t progId = 0; // プログラムID
		uint8_t bankSelectMSB = 0; // バンクセレクト
		uint8_t bankSelectLSB = 0; // バンクセレクト
		float pan = 0.5f; // パン
		float volume = 1.0f; // チャネルボリューム
		float expression = 1.0f; // エクスプレッション
		float pitchBend = 0.0f; // ピッチベンド(ピッチベンドセンシティビティ考慮済み)
		size_t poly = 0; // 同時発音数
		uint8_t attackTime = 64; // アタックタイム
		uint8_t decayTime = 64; // ディケイタイム
		uint8_t releaseTime = 64; // リリースタイム
		bool pedal = false; // ペダルOn/Off
		bool drum = false; // ドラムか否か

		std::unordered_map<VoiceId, Voice::Info> voiceInfo;
	};

	MidiChannel(uint32_t sampleFreq, uint8_t ch, const WaveTable& waveTable);

	void reset(LSP::MIDI::SystemType type);
	void resetParameterNumberState();
	// ---
	void noteOn(uint32_t noteNo, uint8_t vel);
	void noteOff(uint32_t noteNo);
	void programChange(uint8_t progId);
	void controlChange(uint8_t ctrlNo, uint8_t value);
	void pitchBend(int16_t pitch);
	void holdOn();
	void holdOff();
	// ---
	std::pair<float,float> update();
	// ---
	Info info()const;
	// ---


	// サンプリング周波数(実行時に動的にセット)
	const uint32_t sampleFreq;
	// チャネル番号(実行時に動的にセット)
	const uint8_t ch;
		
	// プログラムチェンジ
	uint8_t progId; // プログラムId
	bool isDrumPart = false;
	std::unique_ptr<LSP::Synth::Voice> createVoice(uint8_t noteNo, uint8_t vel);

	// コントロールチェンジ
	uint8_t ccPrevCtrlNo;
	uint8_t ccPrevValue;
	uint8_t ccBankSelectMSB;// CC:0 - バンクセレクトMSB
	float ccVolume;			// CC:7 - チャネル簿リュ－無;
	float ccPan;			// CC:10 - パン [0.0(左), +1.0(右)]
	float ccExpression;		// CC:11 - エクスプレッション [0.0, +1.0]
	uint8_t ccBankSelectLSB;// CC:32 - バンクセレクトLSB
	uint8_t ccReleaseTime;	// CC:72 - リリースタイム
	uint8_t ccAttackTime;	// CC:73 - アタックタイム
	uint8_t ccDecayTime;	// CC:75 - ディケイタイム


	// ピッチベンド
	int16_t cmPitchBend;
	float calculatedPitchBend;
	void applyPitchBend();

	// RPN/NRPN State
	std::optional<uint8_t> ccRPN_MSB;
	std::optional<uint8_t> ccRPN_LSB;
	std::optional<uint8_t> ccNRPN_MSB;
	std::optional<uint8_t> ccNRPN_LSB;
	std::optional<uint8_t> ccDE_MSB;
	std::optional<uint8_t> ccDE_LSB;

	// RPN
	int16_t rpnPitchBendSensitibity;
	bool    rpnNull;

private:
	void voice_noteOn(VoiceId id, uint32_t noteNo, uint8_t vel);
	void voice_noteOff(VoiceId id);

private:
	// 波形テーブル
	const WaveTable& _waveTable;
	// 発音状態管理
	LSP::Synth::VoiceMapper _voiceMapper;
	// ボイス生成
	std::unordered_map<VoiceId, std::unique_ptr<LSP::Synth::Voice>> _voices;
};

}