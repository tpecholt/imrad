set(FA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/fa)

add_library(fa INTERFACE)

target_include_directories(fa
INTERFACE
	${FA_INCLUDE_DIR})

install(FILES 
	"${FA_INCLUDE_DIR}/fa-solid-900.ttf"
	"${FA_INCLUDE_DIR}/fa-regular-400.ttf"
	"${FA_INCLUDE_DIR}/MaterialIcons-Regular.ttf"
	"${FA_INCLUDE_DIR}/Roboto-Medium.ttf"
	"${FA_INCLUDE_DIR}/Roboto-Regular.ttf"
	DESTINATION "style/")