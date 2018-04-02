#include <LSP/Base/Signal.hpp>

using namespace LSP;

static_assert(is_sample_type_v<int8_t>,  "is_sample_type_v failed");
static_assert(is_sample_type_v<int16_t>, "is_sample_type_v failed");
static_assert(is_sample_type_v<int32_t>, "is_sample_type_v failed");
static_assert(is_sample_type_v<float>,   "is_sample_type_v failed");
static_assert(is_sample_type_v<double>,  "is_sample_type_v failed");

static_assert(!is_sample_type_v<uint32_t>,    "is_sample_type_v failed");
static_assert(!is_sample_type_v<long double>, "is_sample_type_v failed");

static_assert(SampleFormatConverter<int8_t,   int8_t>::convert(0) == 0, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int8_t,   int8_t>::convert(0) == 0, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int16_t, int16_t>::convert(0) == 0, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int32_t, int32_t>::convert(0) == 0, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<float,     float>::convert(0.0f) == 0.0f, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<double,   double>::convert(0.0) == 0.0, "SampleFormatConverter failed");

static_assert(SampleFormatConverter<int8_t,   int16_t>::convert(+0x01) == +0x00000100, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int8_t,   int32_t>::convert(+0x01) == +0x01000000, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int8_t,   int16_t>::convert(-0x01) == -0x00000100, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int8_t,   int32_t>::convert(-0x01) == -0x01000000, "SampleFormatConverter failed");

static_assert(SampleFormatConverter<int32_t,  int16_t>::convert(+0x01000000) == +0x0100, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int32_t,   int8_t>::convert(+0x01000000) == +0x01,   "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int32_t,  int16_t>::convert(-0x01000000) == -0x0100, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int32_t,   int8_t>::convert(-0x01000000) == -0x01,   "SampleFormatConverter failed");

static_assert(SampleFormatConverter<float,   double>::convert(+2.0f) == +2.0, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<float,   double>::convert(-2.0f) == -2.0, "SampleFormatConverter failed");

static_assert(SampleFormatConverter<int32_t,   float>::convert(0) == 0.0f, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int32_t,  double>::convert(0) == 0.0,  "SampleFormatConverter failed");
static_assert(SampleFormatConverter<float,   int32_t>::convert(0.0f) == 0, "SampleFormatConverter failed");
static_assert(SampleFormatConverter<double,  int32_t>::convert(0.0) == 0,  "SampleFormatConverter failed");

static_assert(SampleFormatConverter<int8_t,  double>::convert(+0x7F) == +1.0,  "SampleFormatConverter failed");
static_assert(SampleFormatConverter<int8_t,  double>::convert(-0x7F) == -1.0,  "SampleFormatConverter failed");

static_assert(SampleFormatConverter<double,  int8_t>::convert(+1.0) == +0x7F,  "SampleFormatConverter failed");
static_assert(SampleFormatConverter<double,  int8_t>::convert(-1.0) == -0x7F,  "SampleFormatConverter failed");

static_assert(SampleNormalizer<double>::normalize(+2.0) == +1.0,  "SampleNormalizer failed");
static_assert(SampleNormalizer<double>::normalize(-2.0) == -1.0,  "SampleNormalizer failed");

static_assert(SampleNormalizer<int8_t>::normalize(+0x7F) == +0x7F,  "SampleNormalizer failed");
static_assert(SampleNormalizer<int8_t>::normalize(-0x7F) == -0x7F,  "SampleNormalizer failed");
static_assert(SampleNormalizer<int8_t>::normalize(-0x80) == -0x7F,  "SampleNormalizer failed");