# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_reflectionprobe LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_reflectionprobe
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

set(qml_resource_files
    "main.qml"
    "qt_logo.png"
    "cubemap_bc1.ktx"
    "OpenfootageNET_garage-1024.hdr"
    "material_simple.frag"
    "snowflake.png"
)

qt_internal_add_resource(manual_test_reflectionprobe "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
