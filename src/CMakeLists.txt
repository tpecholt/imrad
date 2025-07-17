project (imrad)

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if (WIN32)
  set(SRC ${SRC} "app.rc")
endif()

add_executable(imrad WIN32
   ${SRC}
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)
target_include_directories(imrad PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

add_definitions(-DIMRAD_WITH_GLFW)
add_definitions(-DIMRAD_WITH_STB)
add_definitions(-DSTBI_WINDOWS_UTF8)

target_link_libraries(imrad
	fa
	glfw
	imgui
	nfd
	stb
	${OPENGL_LIBRARIES}
	${CMAKE_DL_LIBS}
	${MISC_FRAMEWORKS}
)

install(FILES "imrad.h" DESTINATION "include/")
install(TARGETS imrad DESTINATION ".")
