#include "test_smf.hpp"

#include <LSP/MIDI/SMF/Parser.hpp>
#include <fstream>

using namespace LSP;

void Test::MidiSmfTest::exec()
{
	Log::d("Testing : MIDI::SMF");

	for (auto file : {"files/Sample0.mid", "files/brambles_vsc3.mid"}) {
		Log::d(LOGF("Testing : MIDI::SMF - parsing file " << file));
		std::ifstream f0(file, std::ios::binary);
		MIDI::SMF::Parser parser;
		[&]()noexcept{parser.parse(f0);}();
	}

	Log::d("Testing : MIDI::SMF - End");
}