set(FA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/fa)

add_library(fa INTERFACE)

target_include_directories(fa
INTERFACE
	${FA_INCLUDE_DIR})

file(GLOB FONT_FILES "${FA_INCLUDE_DIR}/*.ttf")
file(COPY ${FONT_FILES} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/style/")

install(FILES ${FONT_FILES} DESTINATION "style/")
