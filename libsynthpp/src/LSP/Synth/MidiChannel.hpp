#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>
#include <LSP/Synth/VoiceMapper.hpp>
#include <LSP/Synth/Voice.hpp>

#include <array>

namespace LSP::Synth
{

class MidiChannel
{
public:
	MidiChannel(uint32_t sampleFreq, uint8_t ch);

	void reset(LSP::MIDI::SystemType type);
	void resetParameterNumberState();
	// ---
	void noteOn(uint32_t noteNo, uint8_t vel);
	void noteOff(uint32_t noteNo);
	void holdOn();
	void holdOff();
	// ---
	std::pair<float,float> update();
	// ---


	// �T���v�����O���g��(���s���ɓ��I�ɃZ�b�g)
	const uint32_t sampleFreq;
	// �`���l���ԍ�(���s���ɓ��I�ɃZ�b�g)
	const uint8_t ch;
		
	// �v���O�����`�F���W
	uint8_t pcId; // �v���O����Id
	LSP::Filter::EnvelopeGenerator<float> pcEG; // �`���l��EG(�p�����[�^�v�Z��)
	void updateProgram();

	// �R���g���[���`�F���W
	uint8_t ccPrevCtrlNo;
	uint8_t ccPrevValue;
	float ccPan;		// CC:10 - �p�� [0.0(��), +1.0(�E)]
	float ccExpression;	// CC:11 - �G�N�X�v���b�V���� [0.0, +1.0]

	// RPN/NRPN State
	std::optional<uint8_t> ccRPN_MSB;
	std::optional<uint8_t> ccRPN_LSB;
	std::optional<uint8_t> ccNRPN_MSB;
	std::optional<uint8_t> ccNRPN_LSB;
	std::optional<uint8_t> ccDE_MSB;
	std::optional<uint8_t> ccDE_LSB;

	// RPN
	int16_t rpnPitchBendSensitibity;
	bool    rpnNull;

private:
	void voice_noteOn(VoiceId id, uint32_t noteNo, uint8_t vel);
	void voice_noteOff(VoiceId id);

private:
	// ������ԊǗ�
	LSP::Synth::VoiceMapper _voiceMapper;
	// �{�C�X����
	std::unordered_map<VoiceId, std::unique_ptr<LSP::Synth::Voice>> _voices;
};

}