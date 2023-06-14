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

# - include
set(SDL2_INCLUDE_DIRS ${sdl2_SOURCE_DIR}/include)
include_directories(${SDL2_INCLUDE_DIRS})

# - lib
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)  
	set(SDL2_LIB_DIRS ${sdl2_SOURCE_DIR}/lib/x64)
else() 
	set(SDL2_LIB_DIRS ${sdl2_SOURCE_DIR}/lib/x86)
endif()
link_directories(${SDL2_LIB_DIRS})


###########################################################
### SDL2_ttf
FetchContent_Declare(
	sdl2_ttf	
	URL https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.20.2-VC.zip
	URL_HASH SHA1=dee48e9c5184c139aa8bcab34a937d1b3df4f503
)
FetchContent_MakeAvailable(sdl2_ttf)

# - include
set(SDL2_ttf_INCLUDE_DIRS ${sdl2_ttf_SOURCE_DIR}/include)

# - lib
include_directories(${SDL2_ttf_INCLUDE_DIRS})
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)  
	set(SDL2_ttf_LIB_DIRS ${sdl2_ttf_SOURCE_DIR}/lib/x64)
else() 
	set(SDL2_ttf_LIB_DIRS ${sdl2_ttf_SOURCE_DIR}/lib/x86)
endif()
link_directories(${SDL2_ttf_LIB_DIRS})


###########################################################
### UmeFont
FetchContent_Declare(
	umefont	
	URL https://ja.osdn.net/projects/ume-font/downloads/22212/umefont_670.7z/
	URL_HASH SHA1=c0f1a0e079ef43dd4ca7756853a0b5fa43e39b2d
)
FetchContent_MakeAvailable(umefont)

set(UmeFont_ROOT_PATH ${umefont_SOURCE_DIR})
