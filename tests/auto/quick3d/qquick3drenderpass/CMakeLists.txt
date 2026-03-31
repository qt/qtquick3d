# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qquick3drenderpass Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquick3drenderpass LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qquick3drenderpass
    SOURCES
        tst_qquick3drenderpass.cpp
    LIBRARIES
        Qt::Quick
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
        Qt::Quick3DHelpersPrivate
    TESTDATA "dummy_imports.qml"
)

# The test loads "import QtQuick" and "import QtQuick3D" via QQmlComponent at
# runtime, which the import scanner cannot discover from the C++ sources. On
# deployment-based platforms such as Android, dummy_imports.qml gives
# qmlimportscanner/androiddeployqt a scannable file so the QML plugins get
# bundled. In static builds the corresponding QML plugins must additionally be
# linked in explicitly (paired with the Q_IMPORT_QML_PLUGIN calls in the test
# source).
if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(tst_qquick3drenderpass PRIVATE
        Qt6::qtquick2plugin
        Qt6::qquick3dplugin
    )
endif()
