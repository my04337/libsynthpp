#pragma once

#include <lsp/base/base.hpp>
#include <lsp/midi/message.hpp>

namespace LSP::Synth
{

// MEMO 標準の信号型はfloatとする


using StereoFrame = std::pair<float, float>;

}