option(FT_DISABLE_BROTLI "" ON)
option(FT_DISABLE_BZIP2 "" ON)
option(FT_DISABLE_HARFBUZZ "" ON)
option(FT_DISABLE_PNG "" ON)
option(FT_DISABLE_ZLIB "" ON)

# Don't set CMAKE_OSX_ARCHITECTURES - let CMake use the compiler's default architecture
# This allows proper handling of:
# - Default: use compiler's native architecture
# - Command line: cmake -DCMAKE_OSX_ARCHITECTURES=x86_64
# - Environment: respect the configured architecture

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/freetype EXCLUDE_FROM_ALL)
