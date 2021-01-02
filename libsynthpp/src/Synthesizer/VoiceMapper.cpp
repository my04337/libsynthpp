#include <LSP/Synth/VoiceMapper.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::Synth;

size_t VoiceMapper::count()const noexcept
{
	return mVoices.size();
}

void VoiceMapper::reset()
{
	mVoices.clear();
	mHold = false;
}

std::pair</*on*/VoiceId, /*off*/VoiceId> VoiceMapper::noteOn(uint32_t noteNo)
{
	std::pair</*on*/VoiceId, /*off*/VoiceId> ret;

	// MEMO : “¯‚¶ƒm[ƒg”Ô†‚Í“¯Žž‚É”­‰¹•s‰Â
	ret.second = _noteOff(noteNo);
	ret.first = _noteOn(noteNo);
	return ret;
}
VoiceId VoiceMapper::noteOff(uint32_t noteNo, bool force)
{
	if (!mHold || force) {
		return _noteOff(noteNo);
	} else {
		auto found = mVoices.find(noteNo);
		if(found != mVoices.end()) {
			found->second.holding = true;
		}
		return {};
	}
}
void VoiceMapper::holdOn()
{
	mHold = true;
}
std::vector</*off*/VoiceId> VoiceMapper::holdOff()
{
	std::vector</*off*/VoiceId> ret;

	for (auto iter = mVoices.begin(); iter != mVoices.end();) {
		if (iter->second.holding) {
			ret.push_back(iter->second.toneId);
			iter = mVoices.erase(iter);
		} else {
			++iter;
		}
	}
	return ret; // NRVO;
}
VoiceId VoiceMapper::_noteOn(uint32_t noteNo)
{
	auto id = VoiceId::issue();
	mVoices[noteNo] = NoteInfo{id, false};
	return id;
}
VoiceId VoiceMapper::_noteOff(uint32_t noteNo)
{
	auto found = mVoices.find(noteNo);
	if(found == mVoices.end()) return {};

	auto ret = found->second.toneId;
	mVoices.erase(found);
	return ret;
}