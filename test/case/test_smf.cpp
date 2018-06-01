#include "test_smf.hpp"

#include <LSP/MIDI/Parser.hpp>

using namespace LSP;

void Test::MidiSmfTest::exec()
{
	Log::d("Testing : MIDI::SMF");

	for (auto file : {"files/Sample0.mid", "files/brambles_vsc3.mid"}) {
		Log::d(LOGF("Testing : MIDI::SMF - parsing file " << file));
		[&]()noexcept{MIDI::Parser::parse(file).second;}();

	}

	Log::d("Testing : MIDI::SMF - End");
}