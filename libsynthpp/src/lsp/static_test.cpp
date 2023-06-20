#include <lsp/base/math.hpp>
#include <lsp/base/signal.hpp>
#include <lsp/effector/biquadratic_filter.hpp>
#include <lsp/effector/envelope_generator.hpp>
#include <lsp/midi/message.hpp>
#include <lsp/midi/parser.hpp>
#include <lsp/midi/sequencer.hpp>
#include <lsp/audio/wav_file_output.hpp>

using namespace lsp;

// ############################################################################
// ### Base/Math
static_assert(math::PI<float>       != 0,  "math::PI<>failed");
static_assert(math::PI<double>      != 0,  "math::PI<> failed");
static_assert(math::PI<long double> != 0,  "math::PI<> failed");

// ############################################################################
// ### Base/Signal 
static_assert(sample_typeable<int8_t>,  "is_sample_type_v failed");
static_assert(sample_typeable<int16_t>, "is_sample_type_v failed");
static_assert(sample_typeable<int32_t>, "is_sample_type_v failed");
static_assert(sample_typeable<float>,   "is_sample_type_v failed");
static_assert(sample_typeable<double>,  "is_sample_type_v failed");

static_assert(!sample_typeable<uint32_t>,    "is_sample_type_v failed");
static_assert(!sample_typeable<long double>, "is_sample_type_v failed");

static_assert( integral_sample_typeable<int32_t>, "is_integral_sample_type_v failed");
static_assert(!integral_sample_typeable<double>,  "is_integral_sample_type_v failed");

static_assert(!floating_sample_typeable<int32_t>, "is_floating_point_sample_type_v failed");
static_assert( floating_sample_typeable<double>,  "is_floating_point_sample_type_v failed");


static_assert(normalize(+2.0) == +1.0,  "Filter::Normalizer failed");
static_assert(normalize(-2.0) == -1.0,  "Filter::Normalizer failed");

static_assert(normalize(static_cast<int8_t>(+0x7F)) == +0x7F,  "Filter::Normalizer failed");
static_assert(normalize(static_cast<int8_t>(-0x7F)) == -0x7F,  "Filter::Normalizer failed");
static_assert(normalize(static_cast<int8_t>(-0x80)) == -0x7F,  "Filter::Normalizer failed");

static_assert(requantize<int8_t>(static_cast<int8_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(requantize<int8_t>(static_cast<int8_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(requantize<int16_t>(static_cast<int16_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(requantize<int32_t>(static_cast<int32_t>(0)) == 0, "Filter::Requantizer failed");
static_assert(requantize<float>(0.0f) == 0.0f, "Filter::Requantizer failed");
static_assert(requantize<double>(0.0) == 0.0, "Filter::Requantizer failed");

static_assert(requantize<int16_t>(static_cast<int8_t>(+0x01)) == +0x00000100, "Filter::Requantizer failed");
static_assert(requantize<int32_t>(static_cast<int8_t>(+0x01)) == +0x01000000, "Filter::Requantizer failed");
static_assert(requantize<int16_t>(static_cast<int8_t>(-0x01)) == -0x00000100, "Filter::Requantizer failed");
static_assert(requantize<int32_t>(static_cast<int8_t>(-0x01)) == -0x01000000, "Filter::Requantizer failed");

static_assert(requantize<int16_t>(static_cast<int32_t>(+0x01000000)) == +0x0100, "Filter::Requantizer failed");
static_assert(requantize< int8_t>(static_cast<int32_t>(+0x01000000)) == +0x01,   "Filter::Requantizer failed");
static_assert(requantize<int16_t>(static_cast<int32_t>(-0x01000000)) == -0x0100, "Filter::Requantizer failed");
static_assert(requantize< int8_t>(static_cast<int32_t>(-0x01000000)) == -0x01,   "Filter::Requantizer failed");

static_assert(requantize<double>(+2.0f) == +2.0, "Filter::Requantizer failed");
static_assert(requantize<double>(-2.0f) == -2.0, "Filter::Requantizer failed");

static_assert(requantize<float>(static_cast<int32_t>(0)) == 0.0f, "Filter::Requantizer failed");
static_assert(requantize<double>(static_cast<int32_t>(0)) == 0.0,  "Filter::Requantizer failed");
static_assert(requantize<int32_t>(0.0f) == 0, "Filter::Requantizer failed");
static_assert(requantize<int32_t>(0.0)  == 0,  "Filter::Requantizer failed");

static_assert(requantize<double>(static_cast<int8_t>(+0x7F)) == +1.0,  "Filter::Requantizer failed");
static_assert(requantize<double>(static_cast<int8_t>(-0x7F)) == -1.0,  "Filter::Requantizer failed");

static_assert(requantize<int8_t>(+1.0) == +0x7F,  "Filter::Requantizer failed");
static_assert(requantize<int8_t>(-1.0) == -0x7F,  "Filter::Requantizer failed");

// ############################################################################
// ### Filter/EnvelopeGenerator
namespace 
{
[[maybe_unused]]
void unused_function_f_eg() {
	effector::EnvelopeGenerator<float> eg_float;
	effector::EnvelopeGenerator<double> eg_double;
	eg_float.setMelodyEnvelope(1, 0, 0, 0, 0);
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
	effector::BiquadraticFilter<float> bqf_float;
	effector::BiquadraticFilter<double> bqf_double;
}
}

// ############################################################################
// ### Audio::WavFileOutput
namespace 
{
[[maybe_unused]]
void unused_function_a_wfo() {
	audio::WavFileOutput out(44100, 16, 2, "");
	out.write(Signal<int8_t>());
	out.write(Signal<int32_t>());
	out.write(Signal<float>());
	out.write(Signal<double>());
}
}