cmake_minimum_required(VERSION 3.10)
project(OSLib C CXX ASM)

# Set C and C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Load the PSP platform configuration
set(CMAKE_SYSTEM_CMAKE_PLATFORM_PATH "${CMAKE_BINARY_DIR}/cmake")
include(${CMAKE_SYSTEM_CMAKE_PLATFORM_PATH}/PSP.cmake)

# Set the output library
set(TARGET_LIB osl)
add_library(${TARGET_LIB} STATIC)

# Specify the output directory for the .a file (project root directory)
set_target_properties(${TARGET_LIB} PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Include directories
include_directories(
    INTERFACE PUBLIC_HEADER  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/libintraFont/include>
    INTERFACE PUBLIC_HEADER  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/libpspmath/include>
    INTERFACE PRIVATE_HEADER $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/giflib>
    src
    src/adhoc
)

# Define the source files
set(SOURCE_DIR src)
set(SFONT_SOURCES ${SOURCE_DIR}/sfont.c)
set(PSPMATH_SOURCES
    lib/libpspmath/src/printMatrixFloat.c
    lib/libpspmath/src/vfpu_srand.c
    lib/libpspmath/src/vfpu_randf.c
    lib/libpspmath/src/vfpu_rand_8888.c
    lib/libpspmath/src/vfpu_identity_matrix.c
    lib/libpspmath/src/vfpu_translate_matrix.c
    lib/libpspmath/src/vfpu_perspective_matrix.c
    lib/libpspmath/src/vfpu_ortho_matrix.c
    lib/libpspmath/src/vfpu_sinf.c
    lib/libpspmath/src/vfpu_cosf.c
    lib/libpspmath/src/vfpu_tanf.c
    lib/libpspmath/src/vfpu_asinf.c
    lib/libpspmath/src/vfpu_acosf.c
    lib/libpspmath/src/vfpu_atanf.c
    lib/libpspmath/src/vfpu_sinhf.c
    lib/libpspmath/src/vfpu_coshf.c
    lib/libpspmath/src/vfpu_tanhf.c
    lib/libpspmath/src/vfpu_sincos.c
    lib/libpspmath/src/vfpu_expf.c
    lib/libpspmath/src/vfpu_logf.c
    lib/libpspmath/src/vfpu_fabsf.c
    lib/libpspmath/src/vfpu_sqrtf.c
    lib/libpspmath/src/vfpu_powf.c
    lib/libpspmath/src/vfpu_fmodf.c
    lib/libpspmath/src/vfpu_fminf.c
    lib/libpspmath/src/vfpu_fmaxf.c
    lib/libpspmath/src/vfpu_ease_in_out.c
    lib/libpspmath/src/vfpu_normalize_vector.c
    lib/libpspmath/src/vfpu_zero_vector.c
    lib/libpspmath/src/vfpu_scale_vector.c
    lib/libpspmath/src/vfpu_add_vector.c
    lib/libpspmath/src/vfpu_envmap_matrix.c
    lib/libpspmath/src/vfpu_sphere_to_cartesian.c
    lib/libpspmath/src/vfpu_quaternion_identity.c
    lib/libpspmath/src/vfpu_quaternion_copy.c
    lib/libpspmath/src/vfpu_quaternion_multiply.c
    lib/libpspmath/src/vfpu_quaternion_normalize.c
    lib/libpspmath/src/vfpu_quaternion_exp.c
    lib/libpspmath/src/vfpu_quaternion_ln.c
    lib/libpspmath/src/vfpu_quaternion_sample_linear.c
    lib/libpspmath/src/vfpu_quaternion_from_euler.c
    lib/libpspmath/src/vfpu_quaternion_to_matrix.c
    lib/libpspmath/src/vfpu_quaternion_sample_hermite.c
    lib/libpspmath/src/vfpu_quaternion_hermite_tangent.c
)

set(GIFLIB_SOURCES
    lib/giflib/gif2rgb.c
    lib/giflib/dgif_lib.c
    lib/giflib/egif_lib.c
    lib/giflib/gif_err.c
    lib/giflib/gifalloc.c
    lib/giflib/quantize.c
)

set(INTRAFONT_SOURCES
    lib/libintraFont/src/intraFont.c
    lib/libintraFont/src/libccc.c
)

set(LIB_SOURCES
    ${SOURCE_DIR}/adhoc/pspadhoc.c
    ${SOURCE_DIR}/audio/audio.c
    ${SOURCE_DIR}/audio/bgm.c
    ${SOURCE_DIR}/audio/media.c
    ${SOURCE_DIR}/audio/mod.c
    ${SOURCE_DIR}/browser.c
    ${SOURCE_DIR}/dialog.c
    ${SOURCE_DIR}/drawing.c
    ${SOURCE_DIR}/image.c
    ${SOURCE_DIR}/image/oslConvertImageTo.c
    ${SOURCE_DIR}/image/oslDrawImage.c
    ${SOURCE_DIR}/image/oslDrawImageBig.c
    ${SOURCE_DIR}/image/oslDrawImageSimple.c
    ${SOURCE_DIR}/image/oslGetImagePixel.c
    ${SOURCE_DIR}/image/oslLockImage.c
    ${SOURCE_DIR}/image/oslMoveImageTo.c
    ${SOURCE_DIR}/image/oslResetImageProperties.c
    ${SOURCE_DIR}/image/oslScaleImage.c
    ${SOURCE_DIR}/image/oslSetDrawBuffer.c
    ${SOURCE_DIR}/image/oslSetImagePixel.c
    ${SOURCE_DIR}/image/oslSwizzleImage.c
    ${SOURCE_DIR}/image/oslUnswizzleImage.c
    ${SOURCE_DIR}/keys.c
    ${SOURCE_DIR}/map.c
    ${SOURCE_DIR}/mem/oslGetRamStatus.c
    ${SOURCE_DIR}/messagebox.c
    ${SOURCE_DIR}/net.c
    ${SOURCE_DIR}/osk.c
    ${SOURCE_DIR}/oslHandleLoadNoFailError.c
    ${SOURCE_DIR}/oslib.c
    ${SOURCE_DIR}/palette.c
    ${SOURCE_DIR}/saveload.c
    ${SOURCE_DIR}/shape.c
    ${SOURCE_DIR}/image/oslLoadImageFile.c
    ${SOURCE_DIR}/image/format/oslLoadImageFileGIF.c
    ${SOURCE_DIR}/image/format/oslLoadImageFileJPG.c
    ${SOURCE_DIR}/image/format/oslLoadImageFilePNG.c
    ${SOURCE_DIR}/image/oslWriteImageFile.c
    ${SOURCE_DIR}/image/format/oslWriteImageFilePNG.c
    ${SOURCE_DIR}/splash/oslShowSplashScreen1.c
    ${SOURCE_DIR}/splash/oslShowSplashScreen2.c
    ${SOURCE_DIR}/stub.S
    ${SOURCE_DIR}/text.c
    ${SOURCE_DIR}/usb.c
    ${SOURCE_DIR}/vfile/vfsFile.c
    ${SOURCE_DIR}/vfile/VirtualFile.c
    ${SOURCE_DIR}/vfpu.c
    ${SOURCE_DIR}/vram_mgr.c
)

# Specify that stub.S is an assembly file
set_source_files_properties(${SOURCE_DIR}/stub.S PROPERTIES LANGUAGE ASM)

# Add sources to the target
target_sources(${TARGET_LIB} PRIVATE ${SFONT_SOURCES} ${PSPMATH_SOURCES} ${GIFLIB_SOURCES} ${INTRAFONT_SOURCES} ${LIB_SOURCES})

# Installation instructions
install(TARGETS ${TARGET_LIB} DESTINATION ${PSPDIR}/lib)

# Include directories installation (headers)
install(FILES lib/libintraFont/include/intraFont.h lib/libintraFont/include/libccc.h
	DESTINATION ${PSPDIR}/include/oslib)

install(FILES lib/libpspmath/include/pspmath.h
	DESTINATION ${PSPDIR}/include/oslib)

install(FILES ${SOURCE_DIR}/adhoc/pspadhoc.h
    DESTINATION ${PSPDIR}/include/oslib/adhoc)

install(FILES ${SOURCE_DIR}/oslmath.h ${SOURCE_DIR}/net.h ${SOURCE_DIR}/browser.h ${SOURCE_DIR}/audio.h
    ${SOURCE_DIR}/bgm.h ${SOURCE_DIR}/dialog.h ${SOURCE_DIR}/drawing.h ${SOURCE_DIR}/keys.h
    ${SOURCE_DIR}/map.h ${SOURCE_DIR}/messagebox.h ${SOURCE_DIR}/osk.h ${SOURCE_DIR}/saveload.h
    ${SOURCE_DIR}/oslib.h ${SOURCE_DIR}/text.h ${SOURCE_DIR}/usb.h ${SOURCE_DIR}/vfpu_ops.h
    ${SOURCE_DIR}/VirtualFile.h ${SOURCE_DIR}/vram_mgr.h ${SOURCE_DIR}/ccc.h ${SOURCE_DIR}/sfont.h
    DESTINATION ${PSPDIR}/include/oslib)

# Add Cppcheck execution
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    set(CPPCHECK_COMMAND
        ${CPPCHECK} --enable=all --inconclusive --quiet
        --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppress
    )
    add_custom_target(
        cppcheck
        COMMAND ${CPPCHECK_COMMAND} ${CMAKE_SOURCE_DIR}/src
        COMMENT "Running Cppcheck..."
    )
else()
    message(WARNING "Cppcheck not found, skipping static analysis.")
endif()

# Add Uncrustify execution
find_program(UNCRUSTIFY uncrustify)

if(UNCRUSTIFY)
    set(UNCRUSTIFY_COMMAND ${UNCRUSTIFY} -c ${CMAKE_SOURCE_DIR}/uncrustify.cfg --no-backup)
    add_custom_target(
        uncrustify
        COMMAND ${UNCRUSTIFY_COMMAND} --replace ${CMAKE_SOURCE_DIR}/src/*.c ${CMAKE_SOURCE_DIR}/src/*.h
        COMMENT "Running Uncrustify to format the code without creating backups..."
    )
else()
    message(WARNING "Uncrustify not found, skipping code formatting.")
endif()
