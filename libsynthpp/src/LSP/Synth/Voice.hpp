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

public:
	virtual ~Voice() {}

	virtual float update() = 0;
	virtual void setPitchBend(float pitchBend) = 0;
	virtual EnvelopeGenerator& envolopeGenerator() = 0;

};

}