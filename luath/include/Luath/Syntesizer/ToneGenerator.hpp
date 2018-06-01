#pragma once

#include <Luath/Base/Base.hpp>
#include <Luath/Syntesizer/Tone.hpp>
#include <LSP/MIDI/Controller.hpp>
#include <LSP/MIDI/Message.hpp>

#include <array>

namespace Luath::Synthesizer
{
class ToneGenerator
	: public LSP::MIDI::Controller
{
public:
	static constexpr uint8_t MAX_CHANNELS = 16;
	enum class SystemType
	{
		GM1,
		GM2,
		GS,
	};

public:
	ToneGenerator(uint32_t sampleFreq, SystemType defaultSystemType = SystemType::GS);
	~ToneGenerator();


	// MIDIメッセージを受信した際にコールバックされます。
	virtual void onMidiMessageReceived(clock::time_point received_time, const std::shared_ptr<const LSP::MIDI::Message>& msg)override;

	// 指定時刻時点までに蓄積されたMIDIメッセージを解釈します
	virtual void play(clock::time_point until)override;

	// MIDIメッセージを元に演奏した結果を返します
	LSP::Signal<float> generate(size_t len);

protected:
	void dispatchMessage(const std::shared_ptr<const LSP::MIDI::Message>& msg);
	void reset(SystemType type);

	// ノートオン
	void noteOn(clock::time_point played_time, uint8_t ch, uint8_t noteNo, uint8_t vel);

	// ノートオフ
	void noteOff(clock::time_point played_time, uint8_t ch, uint8_t noteNo, uint8_t vel);

	// コントロールチェンジ
	void controlChange(clock::time_point played_time, uint8_t ch, uint8_t ctrlNo, uint8_t value);

	// システムエクスクルーシブ
	void sysExMessage(clock::time_point played_time, const uint8_t* data, size_t len);


private:
	std::recursive_mutex mMutex;
	std::pmr::synchronized_pool_resource mMem;
	std::deque<std::pair<clock::time_point, std::shared_ptr<const LSP::MIDI::Message>>> mMessageQueue;
	SystemType mSystemType;

	// all channel parameters

	// per channel parameters

	struct PerChannelParams 
	{
		PerChannelParams(uint32_t sampleFreq, uint8_t ch);

		void reset(SystemType type);
		void resetParameterNumberState();
		// ---
		void noteOn(uint32_t noteNo, uint8_t vel);
		void noteOff(uint32_t noteNo);
		void holdOn();
		void holdOff();
		// ---
		std::pair<float,float> update();
		// ---


		// サンプリング周波数(実行時に動的にセット)
		const uint32_t sampleFreq;
		// チャネル番号(実行時に動的にセット)
		const uint8_t ch;
		
		// プログラムチェンジ
		uint8_t pcId; // プログラムId
		LSP::Filter::EnvelopeGenerator<float> pcEG; // チャネルEG(パラメータ計算済)
		void updateProgram();

		// コントロールチェンジ
		uint8_t ccPrevCtrlNo;
		uint8_t ccPrevValue;
		float ccPan;		// CC:10 - パン [0.0(左), +1.0(右)]
		float ccExpression;	// CC:11 - エクスプレッション [0.0, +1.0]

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
		void tone_noteOn(ToneId id, uint32_t noteNo, uint8_t vel);
		void tone_noteOff(ToneId id);

	private:
		// 発音状態管理
		LSP::MIDI::ToneMapper _toneMapper;
		// トーン生成
		std::unordered_map<ToneId, std::unique_ptr<Tone>> _tones;
	};
	std::vector<PerChannelParams> mPerChannelParams;
};

}