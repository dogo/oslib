function(osl_add_sample target title source)
    add_executable(${target} ${source})

    execute_process(COMMAND psp-config --psp-prefix OUTPUT_VARIABLE PSPDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(PSPDIR)
        target_link_libraries(${target} PRIVATE "-L${PSPDIR}/sdk/lib" "-L${PSPDIR}/lib")
    endif()
    target_link_libraries(${target} PRIVATE
        osl
        png jpeg z m
        pspdisplay pspge pspuser pspdebug
        pspsdk pspctrl psphprm pspumd psprtc pspgu pspgum pspaudiolib pspaudio pspaudiocodec psppower pspusb pspusbstor psputility pspssl psphttp pspwlan
        ${ARGN}
    )

    set(OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}")
    find_program(MKSFOEX mksfoex)
    find_program(PACKPBP pack-pbp)
    set(PARAM_SFO "${OUT_DIR}/PARAM.SFO")
    set(EBOOT_PBP "${OUT_DIR}/EBOOT.PBP")
    if(MKSFOEX AND PACKPBP)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${MKSFOEX} "${title}" "${PARAM_SFO}"
            COMMAND ${PACKPBP} "${EBOOT_PBP}" "${PARAM_SFO}" NULL NULL NULL NULL NULL $<TARGET_FILE:${target}> NULL
            BYPRODUCTS "${PARAM_SFO}" "${EBOOT_PBP}"
            COMMENT "Packaging EBOOT.PBP (ELF)" VERBATIM)
    endif()

    file(GLOB SAMPLE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
    foreach(F ${SAMPLE_FILES})
        string(TOLOWER "${F}" F_LC)
        if(F_LC MATCHES "\\.(c|cpp|h|hpp|o|elf|prx|pbp)$" OR F_LC STREQUAL "makefile")
            continue()
        endif()
        set(F_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${F}")
        if(IS_DIRECTORY "${F_PATH}")
            # The active build may not have a cache on the first configure.
            string(FIND "${CMAKE_CURRENT_BINARY_DIR}/" "${F_PATH}/" BUILD_DIR_PREFIX)
            file(GLOB_RECURSE BUILD_CACHE_FILES "${F_PATH}/CMakeCache.txt")
            if(BUILD_DIR_PREFIX EQUAL 0 OR BUILD_CACHE_FILES)
                continue()
            endif()
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${F_PATH}" "${OUT_DIR}/${F}" VERBATIM)
        else()
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${F_PATH}" "${OUT_DIR}/${F}" VERBATIM)
        endif()
    endforeach()
endfunction()
