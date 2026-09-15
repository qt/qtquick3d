# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_volumetricfog_computepass LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_volumetricfog_computepass
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::Quick3DPrivate
)

qt_add_qml_module(manual_test_volumetricfog_computepass
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
        SettingsPane.qml
    SOURCES
        path3d.h path3d.cpp
    RESOURCES
        volumetricLight.frag
        compute_shader.comp
        blue_noise.png
    NO_RESOURCE_TARGET_PATH
    IMPORTS
        QtQuick3D
)

