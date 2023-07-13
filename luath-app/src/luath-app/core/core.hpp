/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <lsp/core/core.hpp>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite_3.h>
#include <wincodec.h>

#include <juce_gui_basics/juce_gui_basics.h>

#ifdef small
#undef small // Win32API : rpcndr.h が small というマクロを定義する大迷惑なことをしたことでビルドエラーが起きる事への対処
#endif

#include <juce_gui_extra/juce_gui_extra.h>


namespace luath::app
{
// 利便性のため、lsp名前空間をusing指定する
using namespace lsp;

// 前方宣言
class Application;


}