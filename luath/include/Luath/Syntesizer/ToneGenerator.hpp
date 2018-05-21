#pragma once

#include <Luath/Base/Base.hpp>
#include <LSP/MIDI/Synthesizer/ToneGenerator.hpp>

#include <array>

namespace Luath::Synthesizer
{

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
	LuathToneGenerator(SystemType defaultSystemType = SystemType::GS);
	~LuathToneGenerator();


	// システムエクスクルーシブ
	virtual void sysExMessage(const uint8_t* data, size_t len)override;

protected:
	void reset(SystemType type);

private:
	SystemType mSystemType;

	// all channel parameters

	// per channel parameters
	struct PerChannelParams 
	{
		void reset(SystemType type);
		// ---

		// RPN
		int16_t rpnPitchBendSensitibity;
		bool    rpnNull;
	};
	std::array<PerChannelParams, MAX_CHANNELS> mPerChannelParams;
};

}