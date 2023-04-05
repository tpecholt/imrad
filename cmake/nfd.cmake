if(MSVC)
set(SRC
	3rdparty/nativefiledialog/src/nfd_win.cpp
	3rdparty/nativefiledialog/src/nfd_common.c
)	
else()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(SRC
	3rdparty/nativefiledialog/src/nfd_gtk.c
	3rdparty/nativefiledialog/src/nfd_common.c
)
endif()

add_library(nfd STATIC
   ${SRC}
) 	
	
target_link_libraries(nfd
	${GTK3_LIBRARIES}
)

target_include_directories(nfd
PUBLIC
	3rdparty/nativefiledialog/src/include
)

include_directories(${GTK3_INCLUDE_DIRS})
