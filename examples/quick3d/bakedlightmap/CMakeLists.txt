# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(bakedlightmap LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/bakedlightmap")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(bakedlightmap
    main.cpp
)

# Do not set WIN32_EXECUTABLE, we want a console
set_target_properties(bakedlightmap PROPERTIES
    MACOSX_BUNDLE TRUE
)

target_link_libraries(bakedlightmap PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(bakedlightmap
    URI Example
    VERSION 1.0
    QML_FILES main.qml Box.qml
    RESOURCES box.mesh
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS bakedlightmap
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
