# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qquick3dbuffermanager Test:
#####################################################################

file(GLOB_RECURSE test_data_glob
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquick3dbuffermanager
    SOURCES
        ../shared/util.cpp ../shared/util.h
        tst_buffermanager.cpp
        gradienttexture.cpp gradienttexture.h
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qquick3dbuffermanager CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquick3dbuffermanager CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt6_add_qml_module(tst_qquick3dbuffermanager
    VERSION 1.0
    URI io.qt.tests.auto.BufferManager
    RESOURCE_PREFIX "/"
)
