# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.20)
project(qmlxr LANGUAGES CXX)

set(QT_ANDROID_DEPLOYMENT_TYPE Debug) # no release package signing, no matter what the build type is

find_package(Qt6 COMPONENTS Core Gui Quick Quick3D Quick3DXr)

qt_standard_project_setup()

qt6_policy(SET QTP0002 NEW)

qt_add_executable(qmlxr
    MANUAL_FINALIZATION
    main.cpp
    android/AndroidManifest.xml
)

target_link_libraries(qmlxr PUBLIC
    Qt::Core
    Qt::GuiPrivate
    Qt::QuickPrivate
    Qt::Quick3DPrivate
    Qt::Quick3DXrPrivate
)

qt_add_qml_module(qmlxr
    URI QmlXrTool
    VERSION 1.0
    QML_FILES
        main.qml
        ActionMapper.qml
        Lazer.qml
        Beam.qml
    RESOURCE_PREFIX
        /
    NO_RESOURCE_TARGET_PATH
)

qt_add_resources(qmlxr "assets"
    PREFIX
        "/"
    FILES
        lazer.mesh
        Lazer_baseColor.png
        Lazer_metalness.png
)

if(ANDROID)
    set_property(TARGET qmlxr APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)
endif()

qt_finalize_executable(qmlxr)
