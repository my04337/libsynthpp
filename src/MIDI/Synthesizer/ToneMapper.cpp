#include <LSP/MIDI/Synthesizer/ToneMapper.hpp>
#include <LSP/Base/Logging.hpp>

using namespace LSP;
using LSP::MIDI::Synthesizer::ToneId;
using LSP::MIDI::Synthesizer::ToneMapper;

size_t ToneMapper::count()const noexcept
{
	std::lock_guard lock(mMutex);

	return mTones.size();
}

void ToneMapper::reset()
{
	std::lock_guard lock(mMutex);

	mTones.clear();
	mHold = false;
}

std::pair</*on*/ToneId, /*off*/ToneId> ToneMapper::noteOn(uint32_t noteNo)
{
	std::lock_guard lock(mMutex);
	std::pair</*on*/ToneId, /*off*/ToneId> ret;

	// MEMO : 同じノート番号は同時に発音不可
	ret.second = _noteOff(noteNo);
	ret.first = _noteOn(noteNo);
	return ret;
}
ToneId ToneMapper::noteOff(uint32_t noteNo, bool force)
{
	std::lock_guard lock(mMutex);
	
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
	std::lock_guard lock(mMutex);

	mHold = true;
}
std::vector</*off*/ToneId> ToneMapper::holdOff()
{
	std::lock_guard lock(mMutex);
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