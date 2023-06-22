#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/signal.hpp>
#include <lsp/base/logging.hpp>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <wincodec.h>

#include <sstream>

namespace luath_gui
{
// いくつかのLSPのクラスは便利なためusingしておく
using clock = lsp::clock;
using non_copy = lsp::non_copy;
using non_copy_move = lsp::non_copy_move;

// 前方宣言
class Application;


}