#pragma once

#include <luath_gui/base/base.hpp>

namespace luath_gui
{
class Application final
	: non_copy_move
{
public:
	Application(int argc, char** argv);
	~Application();

	int exec();

private:	
	static LRESULT CALLBACK wndProcProxy(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

}