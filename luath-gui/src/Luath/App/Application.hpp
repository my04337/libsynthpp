#pragma once

#include <Luath/Base/Base.hpp>

namespace Luath
{
class Application final
	: non_copy_move
{
public:
	static Application& instance();

	int exec(int argc, char** argv);

	FontCache& fontCache()noexcept;

private:
	Application();
	~Application();

	std::unique_ptr<FontCache> mFontCache;
};

}