
#quick3d version of qrc to resource
function(quick3d_generate_resource_from_qrc target qrcfile)
    set(generatedResourceFile ${CMAKE_CURRENT_BINARY_DIR}/.rcc/generated_${qrcfile}.qrc )
    set(generatedSourceCode ${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${qrcfile}.cpp )
    set(rccArgs --name ${qrcfile} --output ${generatedSourceCode}  ${generatedResourceFile})

    if(NOT QT_FEATURE_zstd)
        list(APPEND rccArgs "--no-zstd")
    endif()

    add_custom_command(
        OUTPUT
            ${generatedSourceCode}
        COMMAND
            ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
        ARGS
            ${rccArgs}
        DEPENDS
            ${qrcfile}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::rcc>
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::shadergen>
        COMMENT
            RCC ${qrcfile} VERBATIM
    )

    get_target_property(type ${target} TYPE )

    target_sources(${target} PRIVATE "${generatedSourceCode}")
endfunction()

#generate shaders from the given files
function(qt6_quick3d_shader_generator_files target resource_name)

    cmake_parse_arguments(arg
        "" "PREFIX" "FILES" ${ARGN}
    )

    set(output_qrc generated_${resource_name}.qrc)
    set(output_dir ${CMAKE_CURRENT_BINARY_DIR}/.rcc)

    add_custom_command(
        OUTPUT ${resource_name}
        ${QT_TOOL_PATH_SETUP_COMMAND}
        COMMAND
            ${QT_CMAKE_EXPORT_NAMESPACE}::shadergen
            -g
            -o ${output_dir}
            -r ${output_qrc}
            -s  ${PROJECT_SOURCE_DIR}
            -f \"${arg_FILES}\"
        DEPENDS
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::shadergen>
    )

    quick3d_generate_resource_from_qrc(${target} ${resource_name})
endfunction()
