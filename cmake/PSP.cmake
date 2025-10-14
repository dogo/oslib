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

# Set the path to the PSP SDK includes and libraries
include_directories(
    $ENV{PSPDEV}/psp/include
    $ENV{PSPDEV}/psp/sdk/include
)
link_directories(
    $ENV{PSPDEV}/psp/lib
    $ENV{PSPDEV}/psp/sdk/lib
)

