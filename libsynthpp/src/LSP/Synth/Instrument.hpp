#pragma once

#include <LSP/Synth/Base.hpp>

namespace LSP::Synth::Instrument
{

// 通常楽器のエンベロープパラメータを返します (volume, attack, hold, decay, fade, release)
auto getDefaultMelodyEnvelopeParams(uint32_t progId, uint32_t bankId) -> std::tuple<
	float, // volume
	float, // attack_time
	float, // hold_time
	float, // decay_time
	float, // sustain_level
	float  // release_level
>;

// ドラムのノート毎のデフォルトパンポットを返します
float getDefaultDrumPan(uint32_t noteNo);

}
