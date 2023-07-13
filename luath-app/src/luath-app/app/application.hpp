/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>

namespace luath::app
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