#pragma once

#include <luath/base/base.hpp>

namespace luath
{
class Application final
	: non_copy_move
{
public:
	Application(int argc, char** argv);
	~Application();

	int exec();

	const std::filesystem::path& programPath()const noexcept;

private:
	std::filesystem::path mProgramPath;
};

}