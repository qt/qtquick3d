# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from quick3d_visual.pro.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    add_subdirectory(smoketest)
    add_subdirectory(simplescene)
else()
    add_subdirectory(rendercontrol)
    if(NOT ANDROID) # QTBUG-102711
    add_subdirectory(multiwindow)
    endif()
    add_subdirectory(buffermanager)
    if(QT_FEATURE_private_tests)
        add_subdirectory(input)
        add_subdirectory(picking)
    endif()
endif()
