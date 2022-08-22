# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(custommaterial LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/custommaterial")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(custommaterial
    main.cpp
)

set_target_properties(custommaterial PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(custommaterial PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(custommaterial
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCES
        maps/OpenfootageNET_lowerAustria01-1024.hdr
        material_customlights.frag
        material_customspecular.frag
        material_distortion.vert
        material_metallic.frag
        material_simple.frag
        material_transparent.frag
        screen.frag
        weirdShape.mesh
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS custommaterial
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
