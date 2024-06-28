project (test1)

add_executable(test1 WIN32
   test1.cpp
) 	
	
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

add_definitions(-DIMRAD_WITH_GLFW)

target_link_libraries(test1
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
target_link_libraries(test1 stdc++fs)
endif()

include_directories(../src)