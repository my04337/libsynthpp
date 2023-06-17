#pragma once

#include <LSP/Synth/Base.hpp>

namespace LSP::Synth::Instrument
{

// ドラムのノート毎のデフォルトパンポットを返します
float getDefaultDrumPan(uint32_t noteNo);

}
