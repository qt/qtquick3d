# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(principledmaterial LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/principledmaterial")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(principledmaterial
    main.cpp
)

set_target_properties(principledmaterial PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(principledmaterial PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(principledmaterial
    URI Example
    VERSION 1.0
    QML_FILES
        BackgroundCurtain.qml
        TextureSourceControl.qml
        BasicsPane.qml
        AlphaPane.qml
        DetailsPane.qml
        MarkdownLabel.qml
        ClearcoatPane.qml
        RefractionPane.qml
        SpecialPane.qml
        VerticalSectionSeparator.qml
        main.qml
    RESOURCES
        maps/OpenfootageNET_garage-1024.hdr
        maps/grid.png
        maps/alpha_gradient.png
        maps/normal_stamp.png
        maps/monkey_ao.jpg
        maps/monkey_thickness.jpg
        maps/noise.png
        maps/curtain_normal.jpg
        maps/small_envmap.jpg
        maps/tilepattern.png
        maps/metallic/basecolor.jpg
        maps/metallic/metallic.jpg
        maps/metallic/normal.jpg
        maps/metallic/roughness.jpg
        meshes/suzanne.mesh
        meshes/curtain.mesh
        meshes/frame.mesh
        meshes/logo_lines.mesh
        meshes/logo_points.mesh
    SOURCES
        imagehelper.h imagehelper.cpp
    NO_RESOURCE_TARGET_PATH
    IMPORTS
        QtQuick3D
)

install(TARGETS principledmaterial
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
