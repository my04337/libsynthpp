cmake_minimum_required(VERSION 3.24)

include(FetchContent)

###########################################################
### SDL2
FetchContent_Declare(
	sdl2	
	URL https://www.libsdl.org/release/SDL2-devel-2.26.5-VC.zip
	URL_HASH SHA1=f040c352af677161200ec07463efe8d1325135e4
)
FetchContent_MakeAvailable(sdl2)
find_package(
	SDL2 REQUIRED
	HINTS ${sdl2_SOURCE_DIR}
)


###########################################################
### SDL2_ttf
FetchContent_Declare(
	sdl2_ttf	
	URL https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.20.2-VC.zip
	URL_HASH SHA1=dee48e9c5184c139aa8bcab34a937d1b3df4f503
)
FetchContent_MakeAvailable(sdl2_ttf)
find_package(
	SDL2_ttf REQUIRED
	HINTS ${sdl2_ttf_SOURCE_DIR}
)
