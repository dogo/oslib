cmake_minimum_required(VERSION 3.10)
project(adhoc C)

add_executable(adhoc main.c)

# Link OSLib and PSP SDK libs (SDK libs last)
target_link_libraries(adhoc PRIVATE
    osl
    png jpeg z m
    pspdisplay pspge pspuser pspdebug
    pspsdk pspctrl psphprm pspumd psprtc pspgu pspgum pspaudiolib pspaudio pspaudiocodec psppower pspusb pspusbstor psputility pspssl psphttp pspwlan
    pspnet pspnet_apctl pspnet_adhoc pspnet_adhocctl pspnet_adhocmatching
)

# Ensure PSP SDK lib search paths
execute_process(COMMAND psp-config --psp-prefix OUTPUT_VARIABLE PSPDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(PSPDIR)
    target_link_directories(adhoc PRIVATE "${PSPDIR}/sdk/lib" "${PSPDIR}/lib")
    target_link_options(adhoc PRIVATE "-L${PSPDIR}/sdk/lib" "-L${PSPDIR}/lib")
endif()

set_target_properties(adhoc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_program(MKSFOEX mksfoex)
find_program(PACKPBP pack-pbp)

set(OUT_DIR "${CMAKE_BINARY_DIR}")
set(PARAM_SFO "${OUT_DIR}/PARAM.SFO")
set(EBOOT_PBP "${OUT_DIR}/EBOOT.PBP")
set(TITLE "Ad hoc sample")

if(MKSFOEX AND PACKPBP)
    add_custom_command(TARGET adhoc POST_BUILD
        COMMAND ${MKSFOEX} "${TITLE}" "${PARAM_SFO}"
        COMMAND ${PACKPBP} "${EBOOT_PBP}" "${PARAM_SFO}" NULL NULL NULL NULL NULL $<TARGET_FILE:adhoc> NULL
        BYPRODUCTS "${PARAM_SFO}" "${EBOOT_PBP}"
        COMMENT "Packaging EBOOT.PBP (ELF)" VERBATIM)
endif()

file(GLOB SAMPLE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(F ${SAMPLE_FILES})
    string(TOLOWER "${F}" F_LC)
    if(F_LC MATCHES "\\.(c|h|o|elf|prx|pbp)$" OR F_LC STREQUAL "makefile")
        continue()
    endif()
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${F}")
        add_custom_command(TARGET adhoc POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${F}" "${OUT_DIR}/${F}" VERBATIM)
    else()
        add_custom_command(TARGET adhoc POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${F}" "${OUT_DIR}/${F}" VERBATIM)
    endif()
endforeach()
