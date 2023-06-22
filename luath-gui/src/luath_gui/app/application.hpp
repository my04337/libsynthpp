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
};

}