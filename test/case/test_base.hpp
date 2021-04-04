#include "../test.hpp"

namespace LSP::Test
{

class BaseTest final
	: public ITest
{
	virtual std::string_view category()override { return "base"; }
	virtual std::string_view command()override { return "base"; }
	virtual std::string_view description() { return "basic test."; }
	virtual void exec()override;
};

}