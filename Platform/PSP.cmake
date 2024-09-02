# Platform/PSP.cmake
# Configuration file for the PSP platform

# Set the C and C++ compilers
set(CMAKE_C_COMPILER psp-gcc)
set(CMAKE_CXX_COMPILER psp-g++)

# Set the assembler
set(CMAKE_ASM_COMPILER psp-gcc)

# Set the PSP architecture (MIPS32R2)
set(CMAKE_SYSTEM_PROCESSOR mips)

# Set the system name
set(CMAKE_SYSTEM_NAME PSP)

# Set the path to the PSP SDK includes and libraries
include_directories(
    $ENV{PSPDEV}/psp/include
    $ENV{PSPDEV}/psp/sdk/include
)
link_directories(
    $ENV{PSPDEV}/psp/lib
    $ENV{PSPDEV}/psp/sdk/lib
)

# Specify that we're cross-compiling
set(CMAKE_CROSSCOMPILING TRUE)

# Set common PSP compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -G0 -Wall -D__PSP__ -DPSP -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -G0 -Wall -D__PSP__ -DPSP -O2")
