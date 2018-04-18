#include <LSP/MIDI/Messages/BasicMessage.hpp>
#include <LSP/MIDI/Messages/ExtraMessage.hpp>
#include <LSP/MIDI/Messages/MetaEvents.hpp>
#include <LSP/MIDI/Messages/SysExMessage.hpp>

#include <LSP/MIDI/Synthesizer/ToneGenerator.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::MIDI::Messages;

void NoteOn::play(Synthesizer::ToneGenerator& gen)const
{
	gen.noteOn(mChannel, mNoteNo, mVelocity);
}

void NoteOff::play(Synthesizer::ToneGenerator& gen)const
{
	gen.noteOff(mChannel, mNoteNo, mVelocity);
}

void ControlChange::play(Synthesizer::ToneGenerator& gen)const
{
	gen.controlChange(mChannel, mCtrlNo, mValue);
}

void ProgramChange::play(Synthesizer::ToneGenerator& gen)const
{
	gen.programChange(mChannel, mProgNo);
}

void PitchBend::play(Synthesizer::ToneGenerator& gen)const
{
	gen.pitchBend(mChannel, mPitch);
}

void SysExMessage::play(Synthesizer::ToneGenerator& gen)const
{
	gen.sysExMessage(&mData[0], mData.size());
}


