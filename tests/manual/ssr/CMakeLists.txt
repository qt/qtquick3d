# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_ssr LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_ssr
    GUI
    SOURCES
        manual_test_ssr.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(ssr_resource_files
    "ManualTestSSR.qml"
    "MovingRing.qml"
)

qt_internal_add_resource(manual_test_ssr "ssr"
    PREFIX
        "/"
    FILES
        ${ssr_resource_files}
)
