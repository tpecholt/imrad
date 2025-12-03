set(ZLIB_BUILD_TESTING OFF)
set(ZLIB_BUILD_SHARED OFF)
set(ZLIB_BUILD_STATIC ON)
set(ZLIB_BUILD_MINIZIP OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/zlib EXCLUDE_FROM_ALL)

#set(MINIZIP_BUILD_SHARED OFF)
#set(MINIZIP_BUILD_STATIC ON)
#set(MINIZIP_BUILD_TESTING OFF)
#set(MINIZIP_ENABLE_BZIP2 OFF)
#set(MINIZIP_INSTALL OFF)
#add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/zlib/contrib/minizip EXCLUDE_FROM_ALL)

set(MINIZIP_DIR ${CMAKE_SOURCE_DIR}/3rdparty/zlib/contrib/minizip)

set(MINIZIP_SRC
	${MINIZIP_DIR}/ioapi.c
	${MINIZIP_DIR}/miniunz.c
	${MINIZIP_DIR}/minizip.c
	${MINIZIP_DIR}/unzip.c
	${MINIZIP_DIR}/zip.c
)
if (WIN32)
    set(MINIZIP_PLATFORM_SRC ${MINIZIP_DIR}/iowin32.c)
endif()

add_library(minizip STATIC ${MINIZIP_SRC} ${MINIZIP_PLATFORM_SRC})

target_include_directories(minizip PUBLIC ${MINIZIP_DIR})

target_link_libraries(minizip PUBLIC zlibstatic)
