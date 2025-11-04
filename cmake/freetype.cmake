option(FT_DISABLE_BROTLI "" ON)
option(FT_DISABLE_BZIP2 "" ON)
option(FT_DISABLE_HARFBUZZ "" ON)
option(FT_DISABLE_PNG "" ON)
option(FT_DISABLE_ZLIB "" ON)

# Force arm64 on macOS
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS" FORCE)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/freetype EXCLUDE_FROM_ALL)
