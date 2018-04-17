#include <LSP/MIDI/Messages/BasicMessage.hpp>
#include <LSP/MIDI/Messages/ExtraMessage.hpp>
#include <LSP/MIDI/Messages/MetaEvents.hpp>
#include <LSP/MIDI/Messages/SysExMessage.hpp>

#include <LSP/MIDI/Sequencer.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::MIDI::Messages;

void NoteOn::play(Sequencer& seq)const
{
	seq.noteOn(mChannel, mNoteNo, mVelocity);
}

void NoteOff::play(Sequencer& seq)const
{
	seq.noteOff(mChannel, mNoteNo, mVelocity);
}

void ControlChange::play(Sequencer& seq)const
{
	seq.controlChange(mChannel, mCtrlNo, mValue);
}

void ProgramChange::play(Sequencer& seq)const
{
	seq.programChange(mChannel, mProgNo);
}

void PitchBend::play(Sequencer& seq)const
{
	seq.pitchBend(mChannel, mPitch);
}

void SysExMessage::play(Sequencer& seq)const
{
	seq.sysExMessage(&mData[0], mData.size());
}


