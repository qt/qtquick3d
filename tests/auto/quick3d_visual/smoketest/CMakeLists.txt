# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from smoketest.pro.

#####################################################################
## qquick3dsmoketest Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*
)

qt_internal_add_test(tst_qquick3dsmoketest
    SOURCES
        ../shared/util.cpp ../shared/util.h
        tst_smoketest.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
   TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:smoketest.pro:<TRUE>:
# OTHER_FILES = "data/view.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquick3dsmoketest CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquick3dsmoketest CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qquick3dsmoketest)
endif()
