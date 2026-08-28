# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(hdrq3d LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(hdrq3d
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::Quick
        Qt::Quick3D
)

qt_internal_add_resource(hdrq3d "qml"
    PREFIX
        "/"
    FILES
        "main.qml"
)

qt_internal_add_shaders(hdrq3d "shaders"
    PREFIX
        "/"
    FILES
        "level.frag"
)
