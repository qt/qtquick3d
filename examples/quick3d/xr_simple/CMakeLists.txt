# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(xr_simple LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

find_package(Qt6 COMPONENTS Core Gui Quick Quick3D Quick3DXr)

qt_standard_project_setup()

qt6_policy(SET QTP0002 NEW)

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/xr_simple")

qt_add_executable(xr_simple
    MANUAL_FINALIZATION
    main.cpp
    android/AndroidManifest.xml
)
set_target_properties(xr_simple PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(xr_simple PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
    Qt::Quick3DXr
)

qt6_add_resources(xr_simple "qml"
    PREFIX
        "/"
    FILES
        "LeftHand.qml"
        "RightHand.qml"
        "main.qml"
)

if(ANDROID)
    set_property(TARGET xr_simple APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)
endif()

qt_finalize_executable(xr_simple)

install(TARGETS xr_simple
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
