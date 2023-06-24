import lsp.core;

#include <lsp.core/win32_headers.h>

using namespace lsp;


void basic_com_ptr::add_ref()
{
	if(_ptr) {
		_ptr->AddRef();
	}
}
void basic_com_ptr::reset()
{
	if(_ptr) {
		_ptr->Release();
		_ptr = nullptr;
	}
}