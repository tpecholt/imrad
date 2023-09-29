project (imrad)

file(GLOB_RECURSE SRC "*.cpp" "*.h")
	
add_executable(imrad WIN32
   ${SRC}
) 	
	
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

add_definitions(-DIMRAD_WITH_GLFW)
add_definitions(-DIMRAD_WITH_STB)

target_link_libraries(imrad
	fa
	nfd
	imgui
	glfw
	stb
	${OPENGL_LIBRARIES}
	${CMAKE_DL_LIBS}
	${MISC_FRAMEWORKS}
)

install(FILES "imrad.h" DESTINATION "include/")
add_dependencies(imrad nativefiledialog)

install(TARGETS imrad DESTINATION ".")