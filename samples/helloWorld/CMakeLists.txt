cmake_minimum_required(VERSION 3.10)
project(helloWorld C)

# Build executable from sample sources
add_executable(helloworld main.c)

# Link against installed OSLib and PSP SDK libs; put SDK libs last
target_link_libraries(helloworld PRIVATE
    osl
    png jpeg z m
    pspdisplay pspge pspuser pspdebug
    pspsdk pspctrl psphprm pspumd psprtc pspgu pspgum pspaudiolib pspaudio pspaudiocodec psppower pspusb pspusbstor psputility pspssl psphttp pspwlan
    pspnet_adhoc pspnet_adhocctl pspnet_adhocmatching
)

# Ensure PSP SDK lib search paths are available to the linker
execute_process(
    COMMAND psp-config --psp-prefix
    OUTPUT_VARIABLE PSPDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(PSPDIR)
    # Some CMake+toolchain combos may ignore link_directories; ensure via explicit -L
    target_link_directories(helloworld PRIVATE "${PSPDIR}/sdk/lib" "${PSPDIR}/lib")
    target_link_options(helloworld PRIVATE "-L${PSPDIR}/sdk/lib" "-L${PSPDIR}/lib")
endif()

# Outputs go to the build directory for this sample
set_target_properties(helloworld PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# PSP packaging tools
find_program(MKSFOEX mksfoex)
find_program(PACKPBP pack-pbp)
find_program(PSP_PRXGEN psp-prxgen)

set(OUT_DIR "${CMAKE_BINARY_DIR}")
set(PARAM_SFO "${OUT_DIR}/PARAM.SFO")
set(EBOOT_PBP "${OUT_DIR}/EBOOT.PBP")
set(TITLE "Hello World")

if(MKSFOEX AND PACKPBP)
    add_custom_command(TARGET helloworld POST_BUILD
        COMMAND ${MKSFOEX} "${TITLE}" "${PARAM_SFO}"
        COMMAND ${PACKPBP} "${EBOOT_PBP}" "${PARAM_SFO}" NULL NULL NULL NULL NULL $<TARGET_FILE:helloworld> NULL
        BYPRODUCTS "${PARAM_SFO}" "${EBOOT_PBP}"
        COMMENT "Packaging EBOOT.PBP (ELF)"
        VERBATIM)
endif()

# Copy assets (non-source files) to OUT_DIR
file(GLOB SAMPLE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(F ${SAMPLE_FILES})
    string(TOLOWER "${F}" F_LC)
    if(F_LC MATCHES "\\.(c|h|o|elf|prx|pbp)$" OR F_LC STREQUAL "makefile")
        continue()
    endif()
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${F}")
        add_custom_command(TARGET helloworld POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/${F}"
                "${OUT_DIR}/${F}"
            VERBATIM)
    else()
        add_custom_command(TARGET helloworld POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${F}"
                "${OUT_DIR}/${F}"
            VERBATIM)
    endif()
endforeach()
