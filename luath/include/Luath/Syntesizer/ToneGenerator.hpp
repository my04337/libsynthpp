#pragma once

#include <Luath/Base/Base.hpp>
#include <Luath/Syntesizer/Tones.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/MIDI/Synthesizer/ToneMapper.hpp>
#include <LSP/MIDI/Synthesizer/ToneGenerator.hpp>

#include <array>

namespace Luath::Synthesizer
{
using ToneId = LSP::MIDI::Synthesizer::ToneId;

class LuathToneGenerator
	: public LSP::MIDI::Synthesizer::ToneGenerator
	, non_copy_move
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
	LuathToneGenerator(uint32_t sampleFreq, SystemType defaultSystemType = SystemType::GS);
	~LuathToneGenerator();

	// ノートオン
	virtual void noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel)override;

	// ノートオフ
	virtual void noteOff(uint8_t ch, uint8_t noteNo, uint8_t vel)override;

	// コントロールチェンジ
	virtual void controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value)override;

	// システムエクスクルーシブ
	virtual void sysExMessage(const uint8_t* data, size_t len)override;

	// ---
	LSP::Signal<float> generate(size_t len);

protected:
	void reset(SystemType type);

private:
	std::mutex mMutex;
	std::pmr::synchronized_pool_resource mMem;
	SystemType mSystemType;

	// all channel parameters

	// per channel parameters

	struct PerChannelParams 
	{
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
		uint32_t sampleFreq;
		// チャネル番号(実行時に動的にセット)
		uint8_t ch;
		
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
		LSP::MIDI::Synthesizer::ToneMapper _toneMapper;
		// トーン生成
		std::unordered_map<ToneId, std::unique_ptr<Tone>> _tones;
	};
	std::array<PerChannelParams, MAX_CHANNELS> mPerChannelParams;
};

}