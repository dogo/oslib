# Host-side unit tests for the parts of OSLib that don't need PSP hardware.
# Standalone project: the root CMakeLists.txt forces the PSP toolchain.
#
#   cmake -S tests/host -B build-host-tests && cmake --build build-host-tests && ctest --test-dir build-host-tests
cmake_minimum_required(VERSION 3.10)
project(OSLibHostTests C)

set(CMAKE_C_STANDARD 11)
set(OSL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

enable_testing()

# Sources are compiled from a copy: a quoted #include "oslib.h" always resolves
# next to the including file first, which would pick src/oslib.h over the stub.
function(osl_host_test name)
	set(copies)
	foreach(src ${ARGN})
		get_filename_component(base ${src} NAME)
		configure_file(${OSL_SRC}/${src} ${CMAKE_CURRENT_BINARY_DIR}/osl_src/${base} COPYONLY)
		list(APPEND copies ${CMAKE_CURRENT_BINARY_DIR}/osl_src/${base})
	endforeach()
	add_executable(${name} ${name}.c ${copies})
	# stubs/ first so its oslib.h shadows the real (PSP-only) one
	target_include_directories(${name} PRIVATE stubs ${CMAKE_CURRENT_SOURCE_DIR} ${OSL_SRC})
	# VRAM addresses are 32-bit integers stored in pointers, as on the PSP
	target_compile_options(${name} PRIVATE -Wall -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
		-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
	target_link_options(${name} PRIVATE -fsanitize=address,undefined)
	add_test(NAME ${name} COMMAND ${name})
endfunction()

osl_host_test(test_vram_mgr vram_mgr.c)
osl_host_test(test_virtualfile vfile/VirtualFile.c)
