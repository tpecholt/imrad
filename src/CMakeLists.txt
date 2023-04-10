project (imrad)

file(GLOB_RECURSE SRC "*.cpp" "*.h")
	
add_executable(imrad WIN32
   ${SRC}
) 	
	
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

target_link_libraries(imrad
	fa
	nfd
	imgui
	glfw
	stb
	${OPENGL_LIBRARIES}
	${CMAKE_DL_LIBS}
)

if (WIN32)
else()
target_link_libraries(imrad stdc++fs)
endif()

install(FILES "imrad.h" DESTINATION "include/")

install(TARGETS imrad DESTINATION ".")