# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_manual_test(manual_test_3d_input
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(qml_resource_files
    "main.qml"
    "InputDebugger.qml"
    "custom.frag"
)

qt_internal_add_resource(manual_test_3d_input "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
