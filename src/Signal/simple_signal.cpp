#include <LSP/Signal/simple_signal.hpp>

using namespace LSP;

Signal::Signal()
{}

Signal::Signal(size_t size) 
	: mData(size)
{}

float_t* Signal::data() 
{
	return &mData[0]; 
}

size_t Signal::size()const 
{
	return mData.size(); 
}

// ---

std::unique_ptr<ISignal> SignalSource::obtain(size_t sz)
{
	return std::make_unique<Signal>(sz);
}