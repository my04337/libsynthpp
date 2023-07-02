/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/synth/voice.hpp>
#include <lsp/midi/system_type.hpp>

#include <array>

namespace lsp::midi::synth
{
class WaveTable;
class LuathSynth;

using StereoFrame = std::pair<float, float>;

class MidiChannel
	: non_copy
{
public:
	struct Digest {
		uint8_t ch = 0; // チャネル
		int progId = 0; // プログラムID
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

		std::unordered_map<VoiceId, Voice::Digest> voices;
	};

	MidiChannel(LuathSynth& synth, uint8_t ch);

	void reset(midi::SystemType type);
	void resetParameters();
	// ---
	void noteOn(int noteNo, float vel);
	void noteOff(int noteNo, bool allowTailOff = true);
	void allNotesOff(bool allowTailOff);
	void programChange(int progId);
	void controlChange(int ctrlNo, int value);
	void pitchBend(int pitch);
	void updateHold();
	void setDrumMode(bool isDrumMode);
	// ---
	StereoFrame update();
	// ---
	Digest digest()const;
	// ---

private:
	// ボイスを生成します
	std::unique_ptr<Voice> createVoice(int noteNo, float vel);
	std::unique_ptr<Voice> createMelodyVoice(int noteNo, float vel);
	std::unique_ptr<Voice> createDrumVoice(int noteNo, float vel);

	void updatePitchBend();

	// RPN and NRPN
	std::optional<int> getInt14RPN(int msb, int lsb)const noexcept; // [-0x2000, +0x1FFF]
	std::optional<int> getInt7RPN(int msb, int lsb)const noexcept;  // [-0x80,   +0x7F]

	std::optional<int> getInt14NRPN(int msb, int lsb)const noexcept;// [-0x2000, +0x1FFF]
	std::optional<int> getInt7NRPN(int msb, int lsb)const noexcept; // [-0x80,   +0x7F]
	
private:
	LuathSynth& mSynth;

	// チャネル番号(実行時に動的にセット)
	const uint8_t mMidiCh;

	// 発音中のボイス
	std::unordered_map<VoiceId, std::unique_ptr<Voice>> mVoices;

	// システムリセット種別
	midi::SystemType mSystemType;
		
	// プログラムチェンジ
	int mProgId; // プログラムId
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
	uint8_t ccReleaseTime;	// CC:72 - リリースタイム
	uint8_t ccAttackTime;	// CC:73 - アタックタイム
	uint8_t ccDecayTime;	// CC:75 - ディケイタイム


	// ピッチベンド
	int16_t mRawPitchBend;
	float mCalculatedPitchBend;

	// RPN/NRPN State
	juce::MidiRPNDetector mRPNDetector;
	std::unordered_map<int, std::pair<int, bool>> mRawRPNs;   // [0x0000, 0x3FFF]
	std::unordered_map<int, std::pair<int, bool>> mRawNRPNs;  // [0x0000, 0x3FFF]

};

}