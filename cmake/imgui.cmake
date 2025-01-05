set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/3rdparty/imgui)
file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/*.cpp)
file(GLOB IMGUI_MISC ${IMGUI_DIR}/misc/cpp/*.cpp)
file(GLOB IMGUI_FT ${IMGUI_DIR}/misc/freetype/*.cpp)
file(GLOB IMGUI_HEADERS ${IMGUI_DIR}/*.h)
                 
add_library(imgui STATIC 
	${IMGUI_SOURCES} 
	${IMGUI_HEADERS}
	${IMGUI_MISC}
	${IMGUI_FT}
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

#add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)
add_definitions(-DIMGUI_ENABLE_FREETYPE)
#add_definitions(-DIMGUI_USE_WCHAR32)

include_directories(
    ${IMGUI_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR})
    
target_link_libraries(imgui
    freetype
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES})
    
set_target_properties(imgui PROPERTIES LINKER_LANGUAGE CXX)

#add_custom_command(TARGET imgui POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy
#	"${IMGUI_DIR}/misc/fonts/Roboto-Medium.ttf"
#	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/style/"
#)
