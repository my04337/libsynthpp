﻿// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <lsp/core/core.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef small
#undef small // Win32API : rpcndr.h が small というマクロを定義する大迷惑なことをしたことでビルドエラーが起きる事への対処
#endif

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_events/juce_events.h>


namespace luath::app
{
// 利便性のため、lsp名前空間をusing指定する
using namespace lsp;

// 前方宣言
class Application;


}