#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP::MIDI::Synthesizer
{

struct _tone_id_tag {};
using ToneId = issuable_id_base_t<_tone_id_tag>;


}