#include <LSP/Signal/Signal.hpp>

using namespace LSP;


Signal::Signal(size_t size) 
	: mData(nullptr)
	, mSize(0)
{
	allocate(size);
}

Signal::~Signal()
{
	if (mData) {
		delete[] mData;
	}
}

float_t* Signal::data() 
{
	return mData; 
}

size_t Signal::size()const 
{
	return mSize; 
}

void Signal::allocate(size_t size) 
{
	if (size > 0) {
		mData = new float_t[size];
		mSize = size;
	}
}

// ---

std::shared_ptr<ISignal> SignalSource::obtain(size_t sz)
{
	return std::make_shared<Signal>(sz);
}