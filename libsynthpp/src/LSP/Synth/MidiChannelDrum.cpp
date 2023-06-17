#include <LSP/Synth/MidiChannel.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::Synth;

std::unique_ptr<LSP::Synth::Voice> LSP::Synth::MidiChannel::createDrumVoice(uint8_t noteNo, uint8_t vel)
{
	float v = 1.f; // volume(adjuster)
	float a = 0.f; // sec
	float h = 0.f; // sec
	float d = 0.f; // sec
	float s = 1.f; // level
	float f = 0.f; // Linear : level/sec, Exp : dBFS/sec
	float r = 0.f; // sec

	switch(noteNo)
	{
	case 24:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 25:
		a = 0.05f;
		d = 1.f;
		s = 0.9f;
		r = 0.5f;
		v = 1.5f;
		break;
	case 26:
	case 27:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 28:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 29:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 1.8f;
		break;
	case 30:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 1.8f;
		break;
	case 31:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.2f;
		v = 3.f;
		break;
	case 32:
	case 33:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 34:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 35:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 4.5f;
		break;
	case 36:
		a = 0.f;
		d = 0.25f;
		s = 0.f;
		r = 0.15f;
		v = 4.5f;
		break;
	case 37:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 38:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 4.f;
		break;
	case 39:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 2.f;
		h = 0.05f;
		break;
	case 40:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 41:
		a = 0.f;
		d = 0.5f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 42:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.2f;
		v = 1.6f;
		h = 0.05f;
		break;
	case 43:
		a = 0.f;
		d = 0.5f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 44:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.2f;
		v = 1.6f;
		h = 0.05f;
		break;
	case 45:
		a = 0.f;
		d = 0.5f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 46:
		a = 0.f;
		d = 0.75f;
		s = 0.f;
		r = 0.1f;
		v = 1.6f;
		h = 0.1f;
		break;
	case 47:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 48:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 49:
		a = 0.f;
		d = 2.f;
		s = 0.f;
		r = 1.f;
		v = 2.f;
		break;
	case 50:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 3.5f;
		break;
	case 51:
		a = 0.f;
		d = 1.5f;
		s = 0.f;
		r = 1.f;
		v = 2.5f;
		break;
	case 52:
	case 53:
		a = 0.f;
		d = 1.f;
		s = 0.f;
		r = 1.f;
		v = 2.f;
		break;
	case 54:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 1.f;
		v = 2.f;
		break;
	case 55:
		a = 0.f;
		d = 1.5f;
		s = 0.f;
		r = 1.f;
		v = 1.6f;
		break;
	case 56:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 3.f;
		break;
	case 57:
		a = 0.f;
		d = 1.5f;
		s = 0.f;
		r = 1.f;
		v = 2.f;
		break;
	case 58:
		a = 0.f;
		d = 1.5f;
		s = 0.f;
		r = 1.f;
		v = 2.f;
		break;
	case 59:
		a = 0.f;
		d = 1.5f;
		s = 0.f;
		r = 1.f;
		v = 1.2f;
		break;
	case 60:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 61:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 62:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.05f;
		v = 3.f;
		break;
	case 63:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 64:
		a = 0.f;
		d = 0.35f;
		s = 0.f;
		r = 0.25f;
		v = 2.f;
		break;
	case 65:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 2.f;
		break;
	case 66:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 2.f;
		break;
	case 67:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 68:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 69:
		a = 0.f;
		d = 0.15f;
		s = 0.f;
		r = 0.1f;
		v = 2.5f;
		break;
	case 70:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 71:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		h = 0.1f;
		break;
	case 72:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		h = 0.3f;
		break;
	case 73:
		a = 0.f;
		d = 0.15f;
		s = 0.f;
		r = 0.1f;
		v = 2.5f;
		break;
	case 74:
		a = 0.f;
		d = 0.1f;
		s = 0.f;
		r = 0.1f;
		v = 1.5f;
		h = 0.3f;
		break;
	case 75:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.1f;
		v = 2.f;
		break;
	case 76:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 77:
		a = 0.f;
		d = 0.3f;
		s = 0.f;
		r = 0.2f;
		v = 2.f;
		break;
	case 78:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 2.f;
		break;
	case 79:
		a = 0.f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		v = 2.f;
		break;
	case 80:
		a = 0.f;
		d = 0.35f;
		s = 0.f;
		r = 0.25f;
		v = 2.f;
		break;
	case 81:
		a = 0.f;
		d = 0.2f;
		s = 0.f;
		r = 0.2f;
		v = 0.7f;
		h = 0.1f;
		break;
	default:
		a = 0.f;
		d = 0.35f;
		s = 0.f;
		r = 0.25f;
		v = 2.f;
		break;
	}
	v *= 1.2f;

	float pan = 0.5f;
	switch(noteNo) {
	case 24: pan = 0.35f; break;
	case 25: pan = 0.50f; break;
	case 26: pan = 0.50f; break;
	case 27: pan = 0.50f; break;
	case 28: pan = 0.42f; break;
	case 29: pan = 0.50f; break;
	case 30: pan = 0.50f; break;
	case 31: pan = 0.50f; break;
	case 32: pan = 0.50f; break;
	case 33: pan = 0.50f; break;
	case 34: pan = 0.50f; break;
	case 35: pan = 0.50f; break;
	case 36: pan = 0.50f; break;
	case 37: pan = 0.50f; break;
	case 38: pan = 0.50f; break;
	case 39: pan = 0.42f; break;
	case 40: pan = 0.50f; break;
	case 41: pan = 0.27f; break;
	case 42: pan = 0.66f; break;
	case 43: pan = 0.36f; break;
	case 44: pan = 0.66f; break;
	case 45: pan = 0.45f; break;
	case 46: pan = 0.66f; break;
	case 47: pan = 0.55f; break;
	case 48: pan = 0.64f; break;
	case 49: pan = 0.66f; break;
	case 50: pan = 0.73f; break;
	case 51: pan = 0.35f; break;
	case 52: pan = 0.35f; break;
	case 53: pan = 0.35f; break;
	case 54: pan = 0.58f; break;
	case 55: pan = 0.42f; break;
	case 56: pan = 0.66f; break;
	case 57: pan = 0.35f; break;
	case 58: pan = 0.22f; break;
	case 59: pan = 0.35f; break;
	case 60: pan = 0.77f; break;
	case 61: pan = 0.77f; break;
	case 62: pan = 0.30f; break;
	case 63: pan = 0.30f; break;
	case 64: pan = 0.35f; break;
	case 65: pan = 0.66f; break;
	case 66: pan = 0.66f; break;
	case 67: pan = 0.22f; break;
	case 68: pan = 0.22f; break;
	case 69: pan = 0.22f; break;
	case 70: pan = 0.19f; break;
	case 71: pan = 0.77f; break;
	case 72: pan = 0.77f; break;
	case 73: pan = 0.73f; break;
	case 74: pan = 0.73f; break;
	case 75: pan = 0.66f; break;
	case 76: pan = 0.77f; break;
	case 77: pan = 0.77f; break;
	case 78: pan = 0.35f; break;
	case 79: pan = 0.35f; break;
	case 80: pan = 0.19f; break;
	case 81: pan = 0.19f; break;
	}

	int resolvedNoteNo = 64;
	switch(noteNo)
	{
	case 24:
		resolvedNoteNo = 64;
		break;
	case 25:
		resolvedNoteNo = 57;
		break;
	case 26:
		resolvedNoteNo = 81;
		break;
	case 27:
		resolvedNoteNo = 64;
		break;
	case 28:
	case 29:
		resolvedNoteNo = 69;
		break;
	case 30:
		resolvedNoteNo = 66;
		break;
	case 31:
	case 32:
		resolvedNoteNo = 69;
		break;
	case 33:
		resolvedNoteNo = 66;
		break;
	case 34:
		resolvedNoteNo = 78;
		break;
	case 35:
	case 36:
		resolvedNoteNo = 32;
		break;
	case 37:
		resolvedNoteNo = 64;
		break;
	case 38:
		resolvedNoteNo = 44;
		break;
	case 39:
		resolvedNoteNo = 57;
		break;
	case 40:
		resolvedNoteNo = 64;
		break;
	case 41:
		resolvedNoteNo = 44;
		break;
	case 42:
		resolvedNoteNo = 64;
		break;
	case 43:
		resolvedNoteNo = 46;
		break;
	case 44:
		resolvedNoteNo = 66;
		break;
	case 45:
		resolvedNoteNo = 48;
		break;
	case 46:
		resolvedNoteNo = 70;
		break;
	case 47:
		resolvedNoteNo = 50;
		break;
	case 48:
		resolvedNoteNo = 53;
		break;
	case 49:
		resolvedNoteNo = 68;
		break;
	case 50:
		resolvedNoteNo = 56;
		break;
	case 51:
		resolvedNoteNo = 66;
		break;
	case 52:
		resolvedNoteNo = 62;
		break;
	case 53:
		resolvedNoteNo = 80;
		break;
	case 54:
		resolvedNoteNo = 100;
		break;
	case 55:
		resolvedNoteNo = 70;
		break;
	case 56:
		resolvedNoteNo = 60;
		break;
	case 57:
		resolvedNoteNo = 72;
		break;
	case 58:
		resolvedNoteNo = 60;
		break;
	case 59:
		resolvedNoteNo = 76;
		break;
	case 60:
		resolvedNoteNo = 70;
		break;
	case 61:
		resolvedNoteNo = 64;
		break;
	case 62:
		resolvedNoteNo = 70;
		break;
	case 63:
		resolvedNoteNo = 67;
		break;
	case 64:
		resolvedNoteNo = 58;
		break;
	case 65:
		resolvedNoteNo = 70;
		break;
	case 66:
		resolvedNoteNo = 64;
		break;
	case 67:
		resolvedNoteNo = 70;
		break;
	case 68:
		resolvedNoteNo = 64;
		break;
	case 69:
		resolvedNoteNo = 70;
		break;
	case 70:
		resolvedNoteNo = 66;
		break;
	case 71:
		resolvedNoteNo = 71;
		break;
	case 72:
		resolvedNoteNo = 69;
		break;
	case 73:
		resolvedNoteNo = 70;
		break;
	case 74:
		resolvedNoteNo = 68;
		break;
	case 75:
		resolvedNoteNo = 76;
		break;
	case 76:
		resolvedNoteNo = 70;
		break;
	case 77:
		resolvedNoteNo = 64;
		break;
	case 78:
		resolvedNoteNo = 70;
		break;
	case 79:
		resolvedNoteNo = 64;
		break;
	case 80:
		resolvedNoteNo = 86;
		break;
	case 81:
		resolvedNoteNo = 86;
		break;
	default:
		resolvedNoteNo = 69;
		break;
	}
	resolvedNoteNo += getNRPN_MSB(24, noteNo).value_or(64) - 64;


	// MEMO 人間の聴覚ではボリュームは対数的な特性を持つため、ベロシティを指数的に補正する
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	float volume = powf(10.f, -20.f * (1.f - vel / 127.f) / 20.f);
	float cutoff_level = 0.001f;
	static const LSP::Filter::EnvelopeGenerator<float>::Curve curveExp3(3.0f);

	Voice::EnvelopeGenerator eg;
	eg.setParam((float)mSampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f, cutoff_level);
	auto wg = mWaveTable.get(WaveTable::Preset::WhiteNoise);
	auto voice = std::make_unique<LSP::Synth::WaveTableVoice>(mSampleFreq, wg, eg, noteNo, mCalculatedPitchBend, volume, ccPedal);
	voice->setPan(pan);
	return voice;
}