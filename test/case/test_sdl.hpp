#include "../test.hpp"

namespace LSP::Test
{

class SDLTest final
	: public ITest
{
	virtual std::string_view category()override { return "audio"; }
	virtual std::string_view command()override { return "sdl"; }
	virtual std::string_view description() { return "SDL output test."; }
	virtual void exec()override;
};

}