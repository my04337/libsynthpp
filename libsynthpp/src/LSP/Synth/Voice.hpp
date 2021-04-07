#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>
#include <LSP/Base/Sample.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>

namespace LSP::Synth
{
// ボイス識別番号
struct _voice_id_tag {};
using VoiceId = issuable_id_base_t<_voice_id_tag>;


// ボイス(あるチャネルの1音) - 基底クラス
class Voice
	: non_copy_move
{
public:
	using EnvelopeGenerator = LSP::Filter::EnvelopeGenerator<float>;
	using EnvelopeState = LSP::Filter::EnvelopeState;

	struct Info {
		float freq = 0; // 基本周波数
		float envelope = 0; // エンベロープジェネレータ出力
		EnvelopeState state = EnvelopeState::Free; // エンベロープジェネレータ ステート
	};


public:
	virtual ~Voice() {}

	virtual Info info()const = 0;
	virtual float update() = 0;
	virtual void setPitchBend(float pitchBend) = 0;
	virtual EnvelopeGenerator& envolopeGenerator() = 0;

};

}