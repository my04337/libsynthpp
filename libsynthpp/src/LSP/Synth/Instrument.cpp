#include <LSP/Synth/Instrument.hpp>

using namespace LSP;


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
