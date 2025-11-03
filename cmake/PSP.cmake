# PSP.cmake
# Configuration file for the PSP platform

if( NOT CMAKE_BUILD_TYPE )
        set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE )
endif()

if(DEFINED _initial_CMAKE_TOOLCHAIN_FILE
   AND NOT _initial_CMAKE_TOOLCHAIN_FILE STREQUAL CMAKE_TOOLCHAIN_FILE)
  message(WARNING "The CMAKE_TOOLCHAIN_FILE cannot be changed")
elseif(DEFINED CMAKE_TOOLCHAIN_FILE)
  set(_initial_CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE INTERNAL "")
endif()

set(CMAKE_CROSSCOMPILING      TRUE)
set(CMAKE_SYSTEM_NAME          PSP)
set(CMAKE_SYSTEM_VERSION         1)
set(CMAKE_SYSTEM_PROCESSOR    mips)
set(CMAKE_C_COMPILER       psp-gcc)
set(CMAKE_CXX_COMPILER     psp-g++)
set(CMAKE_ASM_COMPILER     psp-gcc)

# PSP Firmware version and defines
add_definitions(-DPSP_FW_VERSION=371 -D__PSP__ -DPSP)

# Get the PSP prefix using psp-config
execute_process(
    COMMAND psp-config --psp-prefix
    OUTPUT_VARIABLE PSPDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Determine PSP root for includes/libs. Prefer psp-config output, fall back to PSPDEV env.
if(PSPDIR)
    set(PSPDEV_ROOT "${PSPDIR}")
elseif(DEFINED ENV{PSPDEV})
    set(PSPDEV_ROOT "$ENV{PSPDEV}/psp")
endif()

# When cross-compiling we want find_package/find_library to search the PSP SDK
# first. Configure CMake search roots and modes so find_* will look into the
# PSPDEV root for includes and libraries rather than the host system paths.
if(PSPDEV_ROOT)
    set(CMAKE_FIND_ROOT_PATH "${PSPDEV_ROOT};${PSPDEV_ROOT}/sdk" CACHE PATH "Search root for PSP cross build" FORCE)
    set(CMAKE_SYSROOT "${PSPDEV_ROOT}" CACHE PATH "Sysroot for PSP cross-compilation" FORCE)
    # Never search programs in the root path (we want host tools for executables)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    # For libraries and includes, prefer only the root path when cross-compiling
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
else()
    message(WARNING "PSPDEV not set: find_package may locate host system libraries instead of PSP SDK ones")
endif()

# Add PSP SDK to prefix path so find_package can locate CMake configs or modules under the SDK
if(PSPDEV_ROOT)
    list(APPEND CMAKE_PREFIX_PATH "${PSPDEV_ROOT}" "${PSPDEV_ROOT}/sdk")
    # If the SDK provides pkgconfig files, prefer them for pkg-config based finders
    if(EXISTS "${PSPDEV_ROOT}/lib/pkgconfig")
        set(ENV{PKG_CONFIG_LIBDIR} "${PSPDEV_ROOT}/lib/pkgconfig")
    endif()
endif()

# Set PSP compile flags
add_compile_options(-G0 -Wall -Wextra -Wno-unused -Wno-strict-prototypes -Wno-missing-prototypes
	-fno-strict-aliasing)
if(CMAKE_BUILD_TYPE STREQUAL Release)
	add_definitions(-DNDEBUG -D_NDEBUG)
	add_compile_options(-O3)
	set_source_files_properties(
		EXTENSION *.cpp *.hpp
		PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti"
	)
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
	add_definitions(-DDEBUG -D_DEBUG)
	add_compile_options(-ggdb -O0 -Wunused -Wstrict-prototypes -Wmissing-prototypes -fverbose-asm)
endif()

# Linker flags
link_libraries(
    -lpspsdk
    -lpspctrl -lpsphprm
    -lpspumd
    -lpsprtc
    -lpspgu -lpspgum
    -lpspaudiolib
    -lpspaudio -lpspaudiocodec
    -lpsppower
    -lpspusb -lpspusbstor
    -lpsputility
    -lpspssl -lpsphttp -lpspwlan
    -lpspnet_adhoc -lpspnet_adhocctl -lpspnet_adhocmatching
    -lpng -ljpeg -lz -lm
)

# Set the path to the PSP SDK includes and libraries (use PSPDEV_ROOT if available)
if(PSPDEV_ROOT)
    include_directories(
        ${PSPDEV_ROOT}/include
        ${PSPDEV_ROOT}/sdk/include
    )
    link_directories(
        ${PSPDEV_ROOT}/lib
        ${PSPDEV_ROOT}/sdk/lib
    )
else()
    include_directories(
        $ENV{PSPDEV}/psp/include
        $ENV{PSPDEV}/psp/sdk/include
    )
    link_directories(
        $ENV{PSPDEV}/psp/lib
        $ENV{PSPDEV}/psp/sdk/lib
    )
endif()
