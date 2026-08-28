# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_principledmaterial LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_principledmaterial
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set(qml_resource_files
    "MaterialControl.qml"
    "main.qml"
)

qt_internal_add_resource(manual_test_principledmaterial "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(materials_resource_files
    "maps/OpenfootageNET_garage-1024.hdr"
    "maps/metallic/basemetal.astc"
    "maps/metallic/normalrough.astc"
)

qt_internal_add_resource(manual_test_principledmaterial "materials"
    PREFIX
        "/"
    FILES
        ${materials_resource_files}
)
