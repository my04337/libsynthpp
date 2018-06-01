#include <LSP/MIDI/Controller.hpp>

using namespace LSP;
using namespace LSP::MIDI;

size_t ToneMapper::count()const noexcept
{
	return mTones.size();
}

void ToneMapper::reset()
{
	mTones.clear();
	mHold = false;
}

std::pair</*on*/ToneId, /*off*/ToneId> ToneMapper::noteOn(uint32_t noteNo)
{
	std::pair</*on*/ToneId, /*off*/ToneId> ret;

	// MEMO : “¯‚¶ƒm[ƒg”Ô†‚Í“¯Žž‚É”­‰¹•s‰Â
	ret.second = _noteOff(noteNo);
	ret.first = _noteOn(noteNo);
	return ret;
}
ToneId ToneMapper::noteOff(uint32_t noteNo, bool force)
{
	if (!mHold || force) {
		return _noteOff(noteNo);
	} else {
		auto found = mTones.find(noteNo);
		if(found != mTones.end()) {
			found->second.holding = true;
		}
		return {};
	}
}
void ToneMapper::holdOn()
{
	mHold = true;
}
std::vector</*off*/ToneId> ToneMapper::holdOff()
{
	std::vector</*off*/ToneId> ret;

	for (auto iter = mTones.begin(); iter != mTones.end();) {
		if (iter->second.holding) {
			ret.push_back(iter->second.toneId);
			iter = mTones.erase(iter);
		} else {
			++iter;
		}
	}
	return ret; // NRVO;
}
ToneId ToneMapper::_noteOn(uint32_t noteNo)
{
	auto id = ToneId::issue();
	mTones[noteNo] = NoteInfo{id, false};
	return id;
}
ToneId ToneMapper::_noteOff(uint32_t noteNo)
{
	auto found = mTones.find(noteNo);
	if(found == mTones.end()) return {};

	auto ret = found->second.toneId;
	mTones.erase(found);
	return ret;
}