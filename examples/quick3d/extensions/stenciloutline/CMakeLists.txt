# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(stenciloutline LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

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
    QML_FILES
        ${qml_resource_files}
    RESOURCES
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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET stenciloutline
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
