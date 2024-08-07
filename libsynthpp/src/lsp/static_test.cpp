﻿// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#include <lsp/core/core.hpp>
#include <lsp/dsp/biquadratic_filter.hpp>
#include <lsp/dsp/envelope_generator.hpp>
#include <lsp/midi/sequencer.hpp>

using namespace lsp;

// ############################################################################
// ### Base/Math
static_assert(math::PI<float>       != 0,  "math::PI<>failed");
static_assert(math::PI<double>      != 0,  "math::PI<> failed");
static_assert(math::PI<long double> != 0,  "math::PI<> failed");

// ############################################################################
// ### Base/Signal 
static_assert(clamp(+2.0) == +1.0,  "Filter::clampr failed");
static_assert(clamp(-2.0) == -1.0,  "Filter::clampr failed");

static_assert(clamp(static_cast<int8_t>(+0x7F)) == +0x7F,  "Filter::clampr failed");
static_assert(clamp(static_cast<int8_t>(-0x7F)) == -0x7F,  "Filter::clampr failed");
static_assert(clamp(static_cast<int8_t>(-0x80)) == -0x7F,  "Filter::clampr failed");

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
	dsp::EnvelopeGenerator<float> eg_float;
	dsp::EnvelopeGenerator<double> eg_double;
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
	dsp::BiquadraticFilter<float> bqf_float;
	dsp::BiquadraticFilter<double> bqf_double;
}
}
