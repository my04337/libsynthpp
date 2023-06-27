#pragma once

#include <luath/core/core.hpp>

namespace luath
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