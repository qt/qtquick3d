# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquick3dskymaterial LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qquick3dskymaterial
    SOURCES
        ../shared/util.cpp ../shared/util.h
        tst_skymaterial.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::GuiPrivate
        Qt::QuickPrivate
        Qt::Quick3D
    TESTDATA
        data/skymaterial_reload.qml
)

qt_internal_extend_target(tst_qquick3dskymaterial CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquick3dskymaterial CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qquick3dskymaterial)
endif()
