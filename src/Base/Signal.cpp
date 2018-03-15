#include <LSP/Base/Signal.hpp>

using namespace LSP;

static_assert(is_sample_type_v<int8_t>,  "is_sample_type_v failed");
static_assert(is_sample_type_v<int16_t>, "is_sample_type_v failed");
static_assert(is_sample_type_v<int32_t>, "is_sample_type_v failed");
static_assert(is_sample_type_v<float>,   "is_sample_type_v failed");
static_assert(is_sample_type_v<double>,  "is_sample_type_v failed");

static_assert(!is_sample_type_v<uint32_t>,    "is_sample_type_v failed");
static_assert(!is_sample_type_v<long double>, "is_sample_type_v failed");
