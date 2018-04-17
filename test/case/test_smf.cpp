#include "test_smf.hpp"

#include <LSP/MIDI/SMF/Player.hpp>
#include <LSP/MIDI/Sequencer.hpp>

using namespace LSP;

void Test::MidiSmfTest::exec()
{
	Log::d("Testing : MIDI::SMF");

	for (auto file : {"files/Sample0.mid", "files/brambles_vsc3.mid"}) {
		Log::d(LOGF("Testing : MIDI::SMF - parsing file " << file));
		MIDI::NullSequencer seq;
		MIDI::SMF::Player player(seq);
		[&]()noexcept{player.open(file);}();
	}

	Log::d("Testing : MIDI::SMF - End");
}