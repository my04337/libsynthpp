#include <LSP/minimal.hpp>

// テンプレート実体化チェック

namespace {

void uncalled_func()
{
	constexpr auto pi1 = LSP::PI<float>;
	constexpr auto pi2 = LSP::PI<double>;
	constexpr auto pi3 = LSP::PI<long double>;
}

}