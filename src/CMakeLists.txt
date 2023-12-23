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

add_dependencies(imrad nativefiledialog)

install(FILES "imrad.h" DESTINATION "include/")
install(TARGETS imrad DESTINATION ".")
install(FILES "icon-40.png" DESTINATION ".")
install(FILES "icon-100.png" DESTINATION ".")
