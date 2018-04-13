#include "../test.hpp"

namespace LSP::Test
{

class MidiSmfTest final
	: public ITest
{
	virtual std::string_view category()override { return "midi"; }
	virtual std::string_view command()override { return "smf"; }
	virtual std::string_view description() { return "smf parser test."; }
	virtual void exec()override;
};

}