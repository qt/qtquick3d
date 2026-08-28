# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_skymaterial LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_skymaterial
    GUI
    SOURCES
        manual_test_skymaterial.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::GuiPrivate
        Qt::Quick3DPrivate
)

# Resources:
set(skymaterial_resource_files
    "advancedsky.frag"
    "AdvancedSky.qml"
    "basicsky.frag"
    "BasicSky.qml"
    "ManualTestSkyMaterial.qml"
    "skytexture.frag"
    "transmittance.frag"
)

qt_internal_add_resource(manual_test_skymaterial "skymaterial"
    PREFIX
        "/"
    FILES
        ${skymaterial_resource_files}
)
