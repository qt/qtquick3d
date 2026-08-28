# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_volumetric LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_volumetric
    GUI
    SOURCES
        main.cpp
        volumetexturedata.cpp
        volumetexturedata.h
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(qml_resource_files
    "colormap.png"
    "custom.frag"
    "custom.vert"
    "main.qml"
    "OpenfootageNET_garage-1024.hdr"
)

qt_internal_add_resource(manual_test_volumetric "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

qt_add_qml_module(manual_test_volumetric
    VERSION 1.0
    URI Volume
    RESOURCE_PREFIX /
)

