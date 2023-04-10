set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/stb)

add_library(stb INTERFACE)

target_include_directories(stb
INTERFACE
	${STB_INCLUDE_DIR})
