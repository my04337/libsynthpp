#include "../test.hpp"

namespace LSP::Test
{

class WasapiTest final
	: public ITest
{
	virtual std::string_view category()override { return "audio"; }
	virtual std::string_view command()override { return "wasapi"; }
	virtual std::string_view description() { return "wasapi output test."; }
	virtual void exec()override;
};

}