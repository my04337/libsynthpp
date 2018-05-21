#include <Luath/Syntesizer/ToneGenerator.hpp>

using namespace LSP;
using Luath::Synthesizer::LuathToneGenerator;

LuathToneGenerator::LuathToneGenerator(SystemType defaultSystemType)
{
	reset(defaultSystemType);
}
LuathToneGenerator::~LuathToneGenerator()
{
}
// 各種パラメータ類 リセット
void LuathToneGenerator::reset(SystemType type)
{
	mSystemType = type;

	for (auto& params : mPerChannelParams) {
		params.reset(type);
	}
}
// チャネル毎パラメータ類 リセット
void LuathToneGenerator::PerChannelParams::reset(SystemType type)
{
	rpnNull = false;
	rpnPitchBendSensitibity = 2;
}

// ---

// システムエクスクルーシブ
void LuathToneGenerator::sysExMessage(const uint8_t* data, size_t len)
{
	size_t pos = 0;
	auto peek = [&](size_t offset = 0) -> std::optional<uint8_t> {
		if(pos+offset >= len) return {};
		return data[pos+offset];
	};
	auto read = [&]() -> std::optional<uint8_t> {
		if(pos >= len) return {};
		return data[pos++];
	};
	auto match = [&](const std::vector<std::optional<uint8_t>>& pattern) -> bool {
		for (size_t i = 0; i < pattern.size(); ++i) {
			auto v = peek(i);
			if(!v.has_value()) return false;
			if(pattern[i].has_value() && pattern[i].value() != v.value()) return false;
		}
		return true;
	};

	auto makerId = read();
	if (makerId == 0x7E) {
		// リアルタイム ユニバーサルシステムエクスクルーシブ
		// https://www.g200kg.com/jp/docs/tech/universalsysex.html

		if (match({0x7F, 0x09, 0x01, 0xF7})) {
			// GM1 System On
			reset(SystemType::GM1);
		} 
		if(match({0x7F, 0x09, 0x03, 0xF7})) {
			// GM2 System On
			reset(SystemType::GM2);
		}
		if (match({0x7F, 0x09, 0x02, 0xF7})) {
			// GM System Off → GS Reset
			reset(SystemType::GS);
		}
	} 
	if (makerId = 0x41) {
		// Roland
		if (match({{/*dev:any*/}, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7})) {
			// GS Reset
			reset(SystemType::GS);
		}
	}
}
