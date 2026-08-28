# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_ssao LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_ssao
    GUI
    SOURCES
        ssao.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(ssao_resource_files
    "custom.frag"
    "custom.vert"
    "custom_unshaded.frag"
    "custom_unshaded.vert"
    "object1.mesh"
    "ssao.qml"
)

qt_internal_add_resource(manual_test_ssao "ssao"
    PREFIX
        "/"
    FILES
        ${ssao_resource_files}
)
