
#quick3d version of qrc to resource
function(_qt_internal_quick3d_generate_resource_from_qrc target qrcfile)
    set(generatedResourceFile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/generated_${qrcfile}.qrc")
    set(generatedSourceCode "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${qrcfile}.cpp")
    set(rccArgs --name "${qrcfile}" --output "${generatedSourceCode}"  "${generatedResourceFile}")

    if(NOT QT_FEATURE_zstd)
        list(APPEND rccArgs "--no-zstd")
    endif()

    add_custom_command(
        OUTPUT
            "${generatedSourceCode}"
        COMMAND
            ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
        ARGS
            ${rccArgs}
        DEPENDS
            "${qrcfile}"
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::rcc>
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::shadergen>
        COMMENT
            RCC ${qrcfile} VERBATIM
    )

    get_target_property(type ${target} TYPE)

    target_sources(${target} PRIVATE "${generatedSourceCode}")
endfunction()

#generate shaders from the given files
function(qt6_quick3d_generate_materials target resource_name)

    cmake_parse_arguments(arg
        "" "PREFIX" "FILES" ${ARGN}
    )

    set(output_qrc "generated_${resource_name}.qrc")
    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/.rcc")

    add_custom_command(
        OUTPUT "${resource_name}"
        ${QT_TOOL_PATH_SETUP_COMMAND}
        COMMAND
            ${QT_CMAKE_EXPORT_NAMESPACE}::shadergen
            -C "${PROJECT_SOURCE_DIR}"
            -o "${output_dir}"
            -r "${output_qrc}"
            \""${arg_FILES}"\"
        DEPENDS
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::shadergen>
    )

    _qt_internal_quick3d_generate_resource_from_qrc(${target} ${resource_name})
endfunction()

# .hdr -> .ktx baker. Quite similar to qt6_add_shaders().
#
# For example, the following autogenerates the lightprobe map at build time and
# includes it in the executable under :/maps/OpenfootageNET_garage-1024.ktx:
#
# qt6_quick3d_bake_lightprobe_hdri(principledmaterial "ibl_assets"
#    PREFIX
#        "/maps"
#    FILES
#        "maps/OpenfootageNET_garage-1024.hdr" )
#
function(qt6_quick3d_bake_lightprobe_hdri target resource_name)
    cmake_parse_arguments(arg
        ""
        "PREFIX;_QT_INTERNAL"
        "FILES"
        ${ARGN}
    )

    foreach(file IN LISTS arg_FILES)
        get_filename_component(file_name_wo_ext ${file} NAME_WLE)
        get_filename_component(file_ext ${file} EXT)
        if(NOT file_ext STREQUAL ".hdr")
            message(FATAL_ERROR "Light probe HDRI maps must have a .hdr extension, whereas ${file} has something else")
        endif()

        set(ktx_result "${CMAKE_CURRENT_BINARY_DIR}/.ibl/${file_name_wo_ext}.ktx")
        get_filename_component(ktx_result_name "${ktx_result}" NAME)

        get_filename_component(file_absolute ${file} ABSOLUTE)

        list(APPEND balsam_args "--no-plugins")
        list(APPEND balsam_args "-o")
        list(APPEND balsam_args "${CMAKE_CURRENT_BINARY_DIR}/.ibl")
        list(APPEND balsam_args "${file_absolute}")

        add_custom_command(
            OUTPUT
                ${ktx_result}
            COMMAND
                ${QT_CMAKE_EXPORT_NAMESPACE}::balsam ${balsam_args}
            DEPENDS
                "${file_absolute}"
                $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::balsam>
            VERBATIM
        )

        list(APPEND ktx_files "${ktx_result}")
        set_source_files_properties("${ktx_result}" PROPERTIES QT_RESOURCE_ALIAS "${ktx_result_name}")
    endforeach()

    if(arg__QT_INTERNAL)
        qt_internal_add_resources(${target} ${resource_name}
            PREFIX
                "${arg_PREFIX}"
            FILES
                "${ktx_files}"
        )
    else()
        qt6_add_resources(${target} ${resource_name}
            PREFIX
                "${arg_PREFIX}"
            FILES
                "${ktx_files}"
        )
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_quick3d_build_shaders)
        qt6_quick3d_generate_materials(${ARGV})
    endfunction()
    function(qt_quick3d_bake_lightprobe_hdri)
        qt6_quick3d_bake_lightprobe_hdri(${ARGV})
    endfunction()
endif()

# for use by Qt modules that need qt_internal_add_resource
function(qt_quick3d_internal_bake_lightprobe_hdri)
    qt6_quick3d_bake_lightprobe_hdri(${ARGV} _QT_INTERNAL)
endfunction()
