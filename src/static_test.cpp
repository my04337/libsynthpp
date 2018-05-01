#include <LSP/Base/Math.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Filter/Requantizer.hpp>
#include <LSP/Filter/Normalizer.hpp>
#include <LSP/Filter/BiquadraticFilter.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>
#include <LSP/MIDI/Message.hpp>
#include <LSP/MIDI/SMF/Parser.hpp>
#include <LSP/MIDI/SMF/Sequencer.hpp>
#include <LSP/MIDI/Synthesizer/ToneGenerator.hpp>
#include <LSP/MIDI/Synthesizer/Tones/SimpleTone.hpp>
#include <LSP/Audio/WavFileOutput.hpp>

using namespace LSP;

// ############################################################################
// ### Base/Math
static_assert(PI<int32_t>     != 0,  "PI<> failed");
static_assert(PI<float>       != 0,  "PI<>failed");
static_assert(PI<double>      != 0,  "PI<> failed");
static_assert(PI<long double> != 0,  "PI<> failed");

// ############################################################################
// ### Base/Signal 
static_assert(is_sample_type_v<int8_t>,  "is_sample_type_v failed");
static_assert(is_sample_type_v<int16_t>, "is_sample_type_v failed");
static_assert(is_sample_type_v<int32_t>, "is_sample_type_v failed");
static_assert(is_sample_type_v<float>,   "is_sample_type_v failed");
static_assert(is_sample_type_v<double>,  "is_sample_type_v failed");

static_assert(!is_sample_type_v<uint32_t>,    "is_sample_type_v failed");
static_assert(!is_sample_type_v<long double>, "is_sample_type_v failed");

static_assert( is_integral_sample_type_v<int32_t>, "is_integral_sample_type_v failed");
static_assert(!is_integral_sample_type_v<double>,  "is_integral_sample_type_v failed");

static_assert(!is_floating_point_sample_type_v<int32_t>, "is_floating_point_sample_type_v failed");
static_assert( is_floating_point_sample_type_v<double>,  "is_floating_point_sample_type_v failed");

// ############################################################################
// ### Filter/Requantizer
static_assert(Filter::Requantizer<int8_t>()(static_cast<int8_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int8_t>()(static_cast<int8_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int16_t>()(static_cast<int16_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int32_t>()(static_cast<int32_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<float>()(0.0f) == 0.0f, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<double>()(0.0) == 0.0, "Filter::Requantizer failed");

static_assert(Filter::Requantizer<int16_t>()(static_cast<int8_t>(+0x01)) == +0x00000100, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int32_t>()(static_cast<int8_t>(+0x01)) == +0x01000000, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int16_t>()(static_cast<int8_t>(-0x01)) == -0x00000100, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int32_t>()(static_cast<int8_t>(-0x01)) == -0x01000000, "Filter::Requantizer failed");

static_assert(Filter::Requantizer<int16_t>()(static_cast<int32_t>(+0x01000000)) == +0x0100, "Filter::Requantizer failed");
static_assert(Filter::Requantizer< int8_t>()(static_cast<int32_t>(+0x01000000)) == +0x01,   "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int16_t>()(static_cast<int32_t>(-0x01000000)) == -0x0100, "Filter::Requantizer failed");
static_assert(Filter::Requantizer< int8_t>()(static_cast<int32_t>(-0x01000000)) == -0x01,   "Filter::Requantizer failed");

static_assert(Filter::Requantizer<double>()(+2.0f) == +2.0, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<double>()(-2.0f) == -2.0, "Filter::Requantizer failed");

static_assert(Filter::Requantizer<float>()(static_cast<int32_t>(0)) == 0.0f, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<double>()(static_cast<int32_t>(0)) == 0.0,  "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int32_t>()(0.0f) == 0, "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int32_t>()(0.0)  == 0,  "Filter::Requantizer failed");

static_assert(Filter::Requantizer<double>()(static_cast<int8_t>(+0x7F)) == +1.0,  "Filter::Requantizer failed");
static_assert(Filter::Requantizer<double>()(static_cast<int8_t>(-0x7F)) == -1.0,  "Filter::Requantizer failed");

static_assert(Filter::Requantizer<int8_t>()(+1.0) == +0x7F,  "Filter::Requantizer failed");
static_assert(Filter::Requantizer<int8_t>()(-1.0) == -0x7F,  "Filter::Requantizer failed");


// ############################################################################
// ### Filter/Normalizer
static_assert(Filter::Normalizer()(+2.0) == +1.0,  "Filter::Normalizer failed");
static_assert(Filter::Normalizer()(-2.0) == -1.0,  "Filter::Normalizer failed");

static_assert(Filter::Normalizer()(static_cast<int8_t>(+0x7F)) == +0x7F,  "Filter::Normalizer failed");
static_assert(Filter::Normalizer()(static_cast<int8_t>(-0x7F)) == -0x7F,  "Filter::Normalizer failed");
static_assert(Filter::Normalizer()(static_cast<int8_t>(-0x80)) == -0x7F,  "Filter::Normalizer failed");

// ############################################################################
// ### Filter/EnvelopeGenerator
namespace 
{
[[maybe_unused]]
void unused_function_f_eg() {
	Filter::EnvelopeGenerator<float> eg_float;
	Filter::EnvelopeGenerator<double> eg_double;
	eg_float.setParam(1, 0, 0, 0, 0);
	eg_float.noteOn();
	eg_float.update();
	eg_double.noteOff();
	eg_double.update();
}
}
// ############################################################################
// ### Filter/BiquadraticFilter
namespace 
{
[[maybe_unused]]
void unused_function_f_bq() {
	Filter::BiquadraticFilter<float> bqf_float;
	Filter::BiquadraticFilter<double> bqf_double;
}
}

// ############################################################################
// ### MIDI/Synthesizer/Tone
namespace 
{
[[maybe_unused]]
void unused_function_m_s_t() {
	MIDI::Synthesizer::SimpleSinTone<int32_t, float> tone_int32({44100}, {}, {});
	MIDI::Synthesizer::SimpleSinTone<float> tone_float({44100}, {}, {});
	MIDI::Synthesizer::SimpleSinTone<double> tone_double({44100}, {}, {});
}
}

// ############################################################################
// ### Audio::WavFileOutput
namespace 
{
[[maybe_unused]]
void unused_function_a_wfo() {
	Audio::WavFileOutput out(44100, 16, 2, "");
	out.write(Signal<int8_t>());
	out.write(Signal<int32_t>());
	out.write(Signal<float>());
	out.write(Signal<double>());
}
}