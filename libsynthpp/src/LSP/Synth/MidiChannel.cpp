#include <LSP/Synth/MidiChannel.hpp>
#include <LSP/Synth/SimpleVoice.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::Synth;

MidiChannel::MidiChannel(uint32_t sampleFreq, uint8_t ch)
	: sampleFreq(sampleFreq)
	, ch(ch)
{
}
// チャネル毎パラメータ類 リセット
void MidiChannel::reset(SystemType type)
{
	_voiceMapper.reset();
	_voices.clear();

	ccPan = 0.5f;
	ccExpression = 1.0;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	rpnNull = false;
	rpnPitchBendSensitibity = 2;

	resetParameterNumberState();

	pcId = 0; // Acoustic Piano
	updateProgram();
}

void MidiChannel::resetParameterNumberState()
{
	ccRPN_MSB.reset();
	ccRPN_LSB.reset();
	ccNRPN_MSB.reset();
	ccNRPN_LSB.reset();
	ccDE_MSB.reset();
	ccDE_LSB.reset();
}
void MidiChannel::noteOn(uint32_t noteNo, uint8_t vel)
{
	auto kvp = _voiceMapper.noteOn(noteNo);
	voice_noteOff(kvp.second);
	if (vel > 0) {
		voice_noteOn(kvp.first, noteNo, vel);
	}
}
void MidiChannel::noteOff(uint32_t noteNo)
{
	auto releasedTone = _voiceMapper.noteOff(noteNo);
	voice_noteOff(releasedTone);
}
void MidiChannel::holdOn()
{
	_voiceMapper.holdOn();
}
void MidiChannel::holdOff()
{
	auto releasedTones = _voiceMapper.holdOff();
	for (auto& toneId : releasedTones) {
		voice_noteOff(toneId);
	}
}
std::pair<float, float> MidiChannel::update()
{
	// オシレータからの出力はモノラル
	float v = 0;
	for (auto iter = _voices.begin(); iter != _voices.end();) {
		v += iter->second->update();
		if (iter->second->envolopeGenerator().isBusy()) {
			++iter;
		}
		else {
			iter = _voices.erase(iter);
		}
	}

	// ステレオ化
	float lch = v * ccPan;
	float rch = v * (1.0f - ccPan);

	return { lch, rch };
}
void MidiChannel::updateProgram()
{
	static const LSP::Filter::EnvelopeGenerator<float>::Curve curveExp3(3.0f);
	switch (pcId) {
	case 0:	// Acoustic Piano
	default:
		pcEG.setParam((float)sampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f);
		break;
	}
}
void MidiChannel::voice_noteOn(VoiceId id, uint32_t noteNo, uint8_t vel)
{
	float toneVolume = (vel / 127.0f);
	float freq = 440 * exp2(((float)noteNo - 69.0f) / 12.0f);

	LSP::Generator::FunctionGenerator<float> fg;
	fg.setSinWave(sampleFreq, freq);
	auto tone = std::make_unique<LSP::Synth::SimpleVoice>(fg, pcEG, toneVolume);
	_voices.emplace(id, std::move(tone));
}
void MidiChannel::voice_noteOff(VoiceId id)
{
	auto found = _voices.find(id);
	if (found == _voices.end()) return;
	auto& voice = *found->second;

	voice.envolopeGenerator().noteOff();
}
