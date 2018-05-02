#pragma once

#include <Luath/Base/Base.hpp>

namespace Luath::Window
{

class MainWindow
	: non_copy_move
{
public:
	MainWindow();
	~MainWindow();

	bool initialize();

	bool valid() const noexcept { return mWindow != nullptr; }
	operator bool()const noexcept { return valid(); }

private:
	SDL_Window* mWindow = nullptr;

};

}