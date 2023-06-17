#include <LSP/Synth/Instrument.hpp>

using namespace LSP;

// 通常楽器のノート毎のデフォルトパンポットを返します (volume, attack, hold, decay, fade, release)
auto LSP::Synth::Instrument::getDefaultMelodyEnvelopeParams(uint32_t progId, uint32_t bankId)->std::tuple<
	float, // volume
	float, // attack_time
	float, // hold_time
	float, // decay_time
	float, // sustain_level
	float  // release_level
> {
	float v = 1.f;
	float a = 0.f; // sec
	float h = 0.f; // sec
	float d = 0.f; // sec
	float s = 1.f; // level
	float r = 0.f; // sec
	switch(progId)
	{
	case 0:
		a = 0.02f;
		d = 3.f;
		s = 0.f;
		r = 1.f;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		a = 0.02f;
		d = 5.f;
		s = 0.f;
		r = 1.f;
		break;
	case 7:
		a = 0.02f;
		d = 6.f;
		s = 0.f;
		r = 0.1f;
		break;
	case 8:
		a = 0.02f;
		d = 4.f;
		s = 0.f;
		r = 1.5f;
		break;
	case 9:
	case 10:
		a = 0.02f;
		d = 4.f;
		s = 0.f;
		r = 2.f;
		break;
	case 11:
		a = 0.02f;
		d = 7.f;
		s = 0.f;
		r = 1.f;
		break;
	case 12:
	case 13:
		a = 0.02f;
		d = 3.f;
		s = 0.f;
		r = 2.f;
		break;
	case 14:
		a = 0.02f;
		d = 7.f;
		s = 0.f;
		r = 5.f;
		break;
	case 15:
		a = 0.02f;
		d = 3.f;
		s = 0.f;
		r = 2.f;
		break;
	case 16:
	case 17:
	case 18:
		a = 0.02f;
		d = 0.5f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 19:
		a = 0.02f;
		d = 0.5f;
		s = 0.7f;
		r = 1.f;
		break;
	case 20:
		a = 0.02f;
		d = 1.f;
		s = 0.5f;
		r = 1.f;
		break;
	case 21:
	case 22:
	case 23:
		a = 0.03f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 24:
		a = 0.02f;
		d = 7.f;
		s = 0.f;
		r = 0.1f;
		break;
	case 25:
		a = 0.02f;
		d = 5.f;
		s = 0.f;
		r = 0.2f;
		break;
	case 26:
		a = 0.02f;
		d = 6.f;
		s = 0.f;
		r = 0.2f;
		break;
	case 27:
	case 28:
		a = 0.02f;
		d = 5.f;
		s = 0.f;
		r = 0.2f;
		break;
	case 29:
		a = 0.02f;
		d = 7.f;
		s = 0.08f;
		r = 0.1f;
		break;
	case 30:
		a = 0.02f;
		d = 7.f;
		s = 0.08f;
		r = 0.1f;
		h = 0.2f;
		break;
	case 31:
		a = 0.02f;
		d = 10.f;
		s = 0.f;
		r = 0.1f;
		h = 0.2f;
		break;
	case 32:
		a = 0.02f;
		d = 5.f;
		s = 0.08f;
		r = 0.1f;
		break;
	case 33:
		a = 0.02f;
		d = 5.f;
		s = 0.03f;
		r = 0.1f;
		break;
	case 34:
		a = 0.02f;
		d = 4.f;
		s = 0.06f;
		r = 0.1f;
		break;
	case 35:
		a = 0.02f;
		d = 10.f;
		s = 0.f;
		r = 0.1f;
		break;
	case 36:
	case 37:
		a = 0.02f;
		d = 3.f;
		s = 0.01f;
		r = 0.1f;
		break;
	case 38:
		a = 0.02f;
		d = 1.5f;
		s = 0.07f;
		r = 0.1f;
		break;
	case 39:
		a = 0.02f;
		d = 5.f;
		s = 0.13f;
		r = 0.1f;
		h = 0.2f;
		break;
	case 40:
		a = 0.03f;
		d = 5.f;
		s = 0.66f;
		r = 0.2f;
		h = 0.2f;
		break;
	case 41:
		a = 0.03f;
		d = 5.f;
		s = 0.66f;
		r = 0.2f;
		h = 0.35f;
		break;
	case 42:
	case 43:
		a = 0.03f;
		d = 1.f;
		s = 0.8f;
		r = 0.2f;
		break;
	case 44:
		a = 0.03f;
		d = 1.f;
		s = 0.9f;
		r = 1.f;
		break;
	case 45:
		a = 0.03f;
		d = 0.5f;
		s = 0.f;
		r = 0.5f;
		break;
	case 46:
		a = 0.02f;
		d = 4.f;
		s = 0.f;
		r = 3.f;
		break;
	case 47:
		a = 0.02f;
		d = 0.3f;
		s = 0.f;
		r = 0.3f;
		v = 3.5f;
		break;
	case 48:
		a = 0.02f;
		d = 0.3f;
		s = 0.8f;
		r = 0.7f;
		break;
	case 49:
		a = 0.03f;
		d = 0.3f;
		s = 0.8f;
		r = 1.2f;
		break;
	case 50:
		a = 0.03f;
		d = 0.3f;
		s = 0.8f;
		r = 0.7f;
		break;
	case 51:
		a = 0.03f;
		d = 0.3f;
		s = 0.8f;
		r = 1.2f;
		break;
	case 52:
		a = 0.03f;
		d = 0.3f;
		s = 0.8f;
		r = 0.7f;
		break;
	case 53:
		a = 0.03f;
		d = 0.3f;
		s = 0.8f;
		r = 0.7f;
		break;
	case 54:
		a = 0.03f;
		d = 0.5f;
		s = 0.8f;
		r = 0.7f;
		break;
	case 55:
		a = 0.02f;
		d = 0.6f;
		s = 0.f;
		r = 0.4f;
		h = 0.1f;
		v = 2.f;
		break;
	case 56:
		a = 0.03f;
		d = 0.5f;
		s = 0.6f;
		r = 0.1f;
		break;
	case 57:
		a = 0.03f;
		d = 4.f;
		s = 0.6f;
		r = 0.1f;
		break;
	case 58:
		a = 0.03f;
		d = 2.f;
		s = 0.6f;
		r = 0.1f;
		break;
	case 59:
		a = 0.03f;
		d = 1.f;
		s = 0.6f;
		r = 0.1f;
		break;
	case 60:
		a = 0.03f;
		d = 2.f;
		s = 0.6f;
		r = 0.1f;
		break;
	case 61:
		a = 0.03f;
		d = 1.5f;
		s = 0.5f;
		r = 0.1f;
		break;
	case 62:
		a = 0.03f;
		d = 1.f;
		s = 0.5f;
		r = 0.1f;
		break;
	case 63:
		a = 0.03f;
		d = 2.f;
		s = 0.5f;
		r = 0.1f;
		break;
	case 64:
	case 65:
	case 66:
	case 67:
	case 68:
		a = 0.03f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 69:
		a = 0.03f;
		d = 0.5f;
		s = 0.8f;
		r = 0.1f;
		break;
	case 70:
	case 71:
		a = 0.03f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 72:
		a = 0.03f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 73:
	case 74:
	case 75:
		a = 0.02f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 76:
		a = 0.1f;
		d = 0.5f;
		s = 0.8f;
		r = 0.1f;
		break;
	case 77:
		a = 0.03f;
		d = 1.f;
		s = 0.8f;
		r = 0.1f;
		break;
	case 78:
		a = 0.03f;
		d = 1.f;
		s = 0.8f;
		r = 0.3f;
		break;
	case 79:
		a = 0.03f;
		d = 1.f;
		s = 0.8f;
		r = 0.1f;
		break;
	case 80:
		a = 0.02f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 81:
		a = 0.02f;
		d = 1.f;
		s = 0.8f;
		r = 0.1f;
		break;
	case 82:
		a = 0.02f;
		d = 1.f;
		s = 0.6f;
		r = 0.1f;
		break;
	case 83:
		a = 0.03f;
		d = 4.f;
		s = 0.25f;
		r = 0.1f;
		break;
	case 84:
		a = 0.03f;
		d = 4.f;
		s = 0.25f;
		r = 0.1f;
		h = 0.03f;
		break;
	case 85:
		a = 0.03f;
		d = 4.f;
		s = 0.56f;
		r = 0.2f;
		break;
	case 86:
		a = 0.03f;
		d = 4.f;
		s = 0.31f;
		r = 0.3f;
		h = 0.15f;
		break;
	case 87:
		a = 0.03f;
		d = 4.5f;
		s = 0.31f;
		r = 0.1f;
		break;
	case 88:
		a = 0.03f;
		d = 3.f;
		s = 0.56f;
		r = 3.f;
		break;
	case 89:
		a = 0.03f;
		d = 3.f;
		s = 0.9f;
		r = 1.f;
		break;
	case 90:
		a = 0.03f;
		d = 3.f;
		s = 0.4f;
		r = 0.8f;
		break;
	case 91:
		a = 0.03f;
		d = 3.f;
		s = 0.9f;
		r = 3.f;
		break;
	case 92:
		a = 0.03f;
		d = 3.f;
		s = 0.9f;
		r = 2.5f;
		break;
	case 93:
		a = 0.3f;
		d = 8.f;
		s = 0.f;
		r = 2.f;
		break;
	case 94:
		a = 0.03f;
		d = 1.f;
		s = 0.9f;
		r = 1.f;
		h = 0.15f;
		break;
	case 95:
		a = 0.3f;
		d = 5.f;
		s = 0.5f;
		r = 2.f;
		break;
	case 96:
		a = 0.03f;
		d = 4.f;
		s = 0.5f;
		r = 2.f;
		break;
	case 97:
		a = 0.03f;
		d = 3.f;
		s = 0.7f;
		r = 3.f;
		h = 0.1f;
		break;
	case 98:
		a = 0.03f;
		d = 4.f;
		s = 0.f;
		r = 3.f;
		break;
	case 99:
		a = 0.03f;
		d = 4.f;
		s = 0.1f;
		r = 2.f;
		break;
	case 100:
		a = 0.03f;
		d = 5.f;
		s = 0.f;
		r = 3.f;
		h = 0.1f;
		break;
	case 101:
		a = 1.f;
		d = 1.f;
		s = 0.9f;
		r = 2.f;
		break;
	case 102:
		a = 0.03f;
		d = 1.f;
		s = 0.9f;
		r = 2.f;
		break;
	case 103:
		a = 0.03f;
		d = 6.f;
		s = 0.04f;
		r = 2.f;
		break;
	case 104:
		a = 0.03f;
		d = 9.f;
		s = 0.f;
		r = 1.f;
		h = 0.03f;
		break;
	case 105:
		a = 0.03f;
		d = 6.f;
		s = 0.f;
		r = 1.f;
		break;
	case 106:
		a = 0.03f;
		d = 3.f;
		s = 0.f;
		r = 1.f;
		break;
	case 107:
		a = 0.03f;
		d = 3.f;
		s = 0.f;
		r = 2.f;
		break;
	case 108:
		a = 0.03f;
		d = 1.5f;
		s = 0.f;
		r = 1.f;
		break;
	case 109:
	case 110:
	case 111:
		a = 0.03f;
		d = 1.f;
		s = 0.7f;
		r = 0.1f;
		break;
	case 112:
		a = 0.03f;
		d = 3.f;
		s = 0.f;
		r = 1.f;
		break;
	case 113:
		a = 0.03f;
		d = 0.4f;
		s = 0.f;
		r = 0.3f;
		break;
	case 114:
		a = 0.03f;
		d = 3.f;
		s = 0.f;
		r = 2.f;
		break;
	case 115:
		a = 0.03f;
		d = 0.5f;
		s = 0.f;
		r = 0.5f;
		break;
	case 116:
		a = 0.03f;
		d = 3.f;
		s = 0.f;
		r = 3.f;
		break;
	case 117:
	case 118:
		a = 0.03f;
		d = 0.8f;
		s = 0.f;
		r = 0.8f;
		break;
	case 119:
		a = 2.f;
		d = 0.1f;
		s = 0.f;
		r = 0.1f;
		break;
	case 120:
		a = 0.03f;
		d = 0.5f;
		s = 0.f;
		r = 0.5f;
		break;
	case 121:
		a = 0.03f;
		d = 0.5f;
		s = 0.f;
		r = 0.4f;
		break;
	case 122:
		a = 0.5f;
		d = 3.f;
		s = 0.f;
		r = 1.5f;
		v = 2.f;
		h = 1.5f;
		break;
	case 123:
		a = 0.5f;
		d = 4.f;
		s = 0.f;
		r = 0.5f;
		v = 2.f;
		h = 0.8f;
		break;
	case 124:
		a = 0.03f;
		d = 1.f;
		s = 0.8f;
		r = 0.1f;
		v = 2.f;
		break;
	case 125:
		a = 3.5f;
		d = 1.f;
		s = 0.8f;
		r = 0.7f;
		v = 2.f;
		break;
	case 126:
		a = 2.f;
		d = 1.f;
		s = 0.8f;
		r = 0.8f;
		v = 2.f;
		break;
	case 127:
		a = 0.03f;
		d = 1.5f;
		s = 0.f;
		r = 1.5f;
		v = 2.5f;
		break;
	}
	return {v,a, h, d, s, r};
}

// ドラムのノート毎のデフォルトパンポットを返します
float LSP::Synth::Instrument::getDefaultDrumPan(uint32_t noteNo)
{
	switch(noteNo){
	case 24: return 0.35f;
	case 25: return 0.50f;
	case 26: return 0.50f;
	case 27: return 0.50f;
	case 28: return 0.42f;
	case 29: return 0.50f;
	case 30: return 0.50f;
	case 31: return 0.50f;
	case 32: return 0.50f;
	case 33: return 0.50f;
	case 34: return 0.50f;
	case 35: return 0.50f;
	case 36: return 0.50f;
	case 37: return 0.50f;
	case 38: return 0.50f;
	case 39: return 0.42f;
	case 40: return 0.50f;
	case 41: return 0.27f;
	case 42: return 0.66f;
	case 43: return 0.36f;
	case 44: return 0.66f;
	case 45: return 0.45f;
	case 46: return 0.66f;
	case 47: return 0.55f;
	case 48: return 0.64f;
	case 49: return 0.66f;
	case 50: return 0.73f;
	case 51: return 0.35f;
	case 52: return 0.35f;
	case 53: return 0.35f;
	case 54: return 0.58f;
	case 55: return 0.42f;
	case 56: return 0.66f;
	case 57: return 0.35f;
	case 58: return 0.22f;
	case 59: return 0.35f;
	case 60: return 0.77f;
	case 61: return 0.77f;
	case 62: return 0.30f;
	case 63: return 0.30f;
	case 64: return 0.35f;
	case 65: return 0.66f;
	case 66: return 0.66f;
	case 67: return 0.22f;
	case 68: return 0.22f;
	case 69: return 0.22f;
	case 70: return 0.19f;
	case 71: return 0.77f;
	case 72: return 0.77f;
	case 73: return 0.73f;
	case 74: return 0.73f;
	case 75: return 0.66f;
	case 76: return 0.77f;
	case 77: return 0.77f;
	case 78: return 0.35f;
	case 79: return 0.35f;
	case 80: return 0.19f;
	case 81: return 0.19f;
	default: return 0.50f;
	}
}
