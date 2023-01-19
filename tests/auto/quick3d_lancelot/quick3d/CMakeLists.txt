# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quick3d.pro.

#####################################################################
## tst_qquick3d Test:
#####################################################################

# Collect test data
list(APPEND test_data "../data")

qt_internal_add_test(tst_qquick3d
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.." # special case
    SOURCES
        ../shared/baselineprotocol.cpp ../shared/baselineprotocol.h
        ../shared/lookup3.cpp
        ../shared/qbaselinetest.cpp ../shared/qbaselinetest.h
        tst_quick3d.cpp
    DEFINES
        QMAKESPEC=""
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Network
    TESTDATA ${test_data}
)

# special case begin
add_dependencies(tst_qquick3d Qt6::Quick3D)
add_dependencies(tst_qquick3d qquick3d_qmlscenegrabber)
# special case end

## Scopes:
#####################################################################

#### Keys ignored in scope 4:.:../shared:../shared/qbaselinetest.pri:WIN32:
# MKSPEC = "$$replace(QMAKESPEC, \\\\, /)"

#### Keys ignored in scope 5:.:../shared:../shared/qbaselinetest.pri:else:
# MKSPEC = "$$QMAKESPEC"
