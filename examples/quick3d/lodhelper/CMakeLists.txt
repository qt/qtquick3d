# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(lodhelper LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/lodhelper")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(lodhelper
    main.cpp
)

set_target_properties(lodhelper PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(lodhelper PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(lodhelper
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCES
        meshes/marble_bust_01_LOD_0.mesh
        meshes/marble_bust_01_LOD_1.mesh
        meshes/marble_bust_01_LOD_2.mesh
        meshes/marble_bust_01_LOD_3.mesh
        maps/baseColor.png
        maps/normal.png
        maps/occlusionRoughnessMetallic.png
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS lodhelper
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
