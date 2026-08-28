# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_userpasses LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_standard_project_setup(REQUIRES 6.8)

if(NOT QT_FEATURE_developer_build)
    message(WARNING "Skipping manual test userpasses, requires developer build")
    return()
endif()

qt_internal_add_manual_test(manual_test_userpasses
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

qt_add_qml_module(manual_test_userpasses
    URI io.qt.tests.manual.userpasses
    VERSION 1.0
    QML_FILES
        Main.qml
        NormalRoughnessPass.qml
        GBufferPass.qml
    RESOURCES
        normal_roughness_augment.glsl
        gbuffer_augment.glsl
        lighting.frag
        lighting.vert
        blitter.frag
        deferredLightingEffect.frag
    NO_CACHEGEN
)
