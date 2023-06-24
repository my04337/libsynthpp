module;

#include <type_traits>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 
#define STRICT 
#define NOMINMAX 
#include <sdkddkver.h>
#include <Windows.h>
#include <atlbase.h>
#include <atlcom.h>
#endif

export module lsp.core:win32;

