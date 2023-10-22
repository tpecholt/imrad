cmake_minimum_required(VERSION 3.10)
project(imrad)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden) # linux: don't export all symbols in .so
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # linux: use -fPIC on .a so they can be used by .so

# make use of runtime directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/latest")

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_USE_MATH_DEFINES)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
  if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    find_library(MISC_FRAMEWORKS UniformTypeIdentifiers)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
  elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(MISC REQUIRED gtk+-3.0 glib-2.0)
      set(MISC_FRAMEWORKS "${MISC_STATIC_LIBRARIES}")
  endif()

    # boost errors
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -fext-numeric-literals")
    # imgui opengl3 warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")
    # strip binaries
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    add_definitions(-DGL_SILENCE_DEPRECATION=1)
endif()

# before includes
install(
    DIRECTORY "template/" 
    DESTINATION "template"
)
install(
    DIRECTORY "style/" 
    DESTINATION "style"
)

include(ExternalProject)
include(cmake/glfw.cmake)
include(cmake/imgui.cmake)
set(NFD_PREFIX 3rdparty/nativefiledialog)
ExternalProject_Add(nativefiledialog
  PREFIX ${NFD_PREFIX}
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${NFD_PREFIX}
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
  -DCMAKE_OSX_ARCHITECTURES:STRING=x86_64\;arm64
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${NFD_PREFIX}/src/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/${NFD_PREFIX}/src/nativefiledialog-build/src/)

include(cmake/fa.cmake)
include(cmake/stb.cmake)

add_subdirectory(src)

file(COPY
        "${CMAKE_CURRENT_SOURCE_DIR}/template"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
file(COPY
        "${CMAKE_CURRENT_SOURCE_DIR}/style"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

