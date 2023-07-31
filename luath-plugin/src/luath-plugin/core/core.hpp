/**
	luath-plugin

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <luath-plugin/core/core.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef small
#undef small // Win32API : rpcndr.h が small というマクロを定義する大迷惑なことをしたことでビルドエラーが起きる事への対処
#endif

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>


namespace luath::plugin
{
	// 利便性のため、lsp名前空間をusing指定する
	using namespace lsp;


}