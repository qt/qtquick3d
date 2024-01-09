# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(stenciloutline LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/extensions/stenciloutline")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(stenciloutline
    main.cpp
)

set(stenciloutline_uri "QtQuick3D.Examples.OutlineRenderExtension")

set_target_properties(stenciloutline PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(stenciloutline PUBLIC
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::GuiPrivate
        Qt::Quick3DPrivate
)

set(qml_resource_files
    "Main.qml"
    "ColorPicker.qml"
    "SectionLayout.qml"
)

set(image_resource_files
    "images/TreeExpanded.png"
    "images/TreeUnexpanded.png"
    "images/grid_8x8.png"
)

set(mesh_resource_files
    "models/suzanne.mesh"
)

qt_add_qml_module(stenciloutline
    VERSION 1.0
    URI ${stenciloutline_uri}
    RESOURCES
        ${qml_resource_files}
        ${image_resource_files}
        ${mesh_resource_files}
    SOURCES
        outlinerenderextension.cpp outlinerenderextension.h
    NO_PLUGIN
)

qt_add_shaders(stenciloutline "baked-shaders"
    FILES
        "shaders/huesaturation.frag"
)

install(TARGETS stenciloutline
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
