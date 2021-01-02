#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>
#include <LSP/Base/Sample.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>

namespace LSP::MIDI::Synthesizer
{
// �{�C�X���ʔԍ�
struct _voice_id_tag {};
using VoiceId = issuable_id_base_t<_voice_id_tag>;


// �g�[��(����`���l����1��) - ���N���X
template <
	typename sample_type
>
class Voice
	: non_copy_move
{
	static_assert(is_sample_type_v<sample_type>);

public:
	using EnvelopeGenerator = LSP::Filter::EnvelopeGenerator<sample_type>;

public:
	virtual ~Voice() {}

	virtual sample_type update() = 0;
	virtual EnvelopeGenerator& envolopeGenerator() = 0;

};

}