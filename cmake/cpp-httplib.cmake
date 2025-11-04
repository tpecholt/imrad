find_package(OpenSSL QUIET)

option(HTTPLIB_USE_OPENSSL_IF_AVAILABLE "" ON)
option(HTTPLIB_USE_BROTLI_IF_AVAILABLE "" OFF)
option(HTTPLIB_USE_ZSTD_IF_AVAILABLE "" OFF)
option(HTTPLIB_USE_ZLIB_IF_AVAILABLE "" OFF)

# override faulty value in msys build
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(TMP ${CMAKE_SYSTEM_VERSION})
  set(CMAKE_SYSTEM_VERSION "10.0.19041.0")
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/cpp-httplib EXCLUDE_FROM_ALL)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CMAKE_SYSTEM_VERSION ${TMP})
endif()

if (TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto) # OpenSSL_FOUND is true even when disabled in cmake-gui
  message(STATUS "OpenSSL configured")
  add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
else()
  message(WARNING "OpenSSL is NOT confiured")
endif() 