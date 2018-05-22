#include <LSP/MIDI/Synthesizer/ToneMapper.hpp>
#include <LSP/Base/Logging.hpp>

using namespace LSP;
using LSP::MIDI::Synthesizer::ToneId;
using LSP::MIDI::Synthesizer::ToneMapper;


void ToneMapper::setCallback(Callback callback) 
{
	mCallback = std::move(callback); 
}

void ToneMapper::reset()
{
	mTones.clear();
	mHold = false;
}
size_t ToneMapper::count()const noexcept
{
	return mTones.size();
}

ToneId ToneMapper::noteOn(uint32_t noteNo, uint8_t vel)
{
	// MEMO : 同じノート番号は同時に発音不可
	_noteOff(noteNo);
	return _noteOn(noteNo, vel);
}
void ToneMapper::noteOff(uint32_t noteNo, bool force)
{
	if (!mHold || force) {
		_noteOff(noteNo);
	} else {
		_setHold(noteNo);
	}
}
void ToneMapper::holdOn()
{
	mHold = true;
}
void ToneMapper::holdOff()
{
	_resetHold();
}
ToneId ToneMapper::_noteOn(uint32_t noteNo, uint8_t vel)
{
	lsp_assert(vel > 0);
	auto id = ToneId::issue();
	mTones[noteNo] = NoteInfo{id, false};
	if(mCallback) mCallback(id, noteNo, vel);
	return id;
}
void ToneMapper::_noteOff(uint32_t noteNo)
{
	auto found = mTones.find(noteNo);
	if(found == mTones.end()) return;
	_noteOff(found);
}
ToneMapper::ToneMap::iterator ToneMapper::_noteOff(ToneMapper::ToneMap::iterator iter)
{
	auto noteNo = iter->first;
	auto id = iter->second.toneId;
	auto retIter = mTones.erase(iter);
	if(mCallback) mCallback(id, noteNo, 0);
	return retIter;
}
void ToneMapper::_setHold(uint32_t noteNo)
{
	auto found = mTones.find(noteNo);
	if(found == mTones.end()) return;

	found->second.holding = true;
}
void ToneMapper::_resetHold()
{
	for (auto iter = mTones.begin(); iter != mTones.end();) {
		if (iter->second.holding) {
			iter = _noteOff(iter);
		} else {
			++iter;
		}
	}
}