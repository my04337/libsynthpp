#pragma once

#include <LSP/minimal.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::Synth
{

// MEMO 標準の信号型はfloatとする


using StereoFrame = std::pair<float, float>;

}