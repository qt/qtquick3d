# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_manual_test(manual_test_reflectionprobe
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

set(qml_resource_files
    "main.qml"
    "qt_logo.png"
    "OpenfootageNET_garage-1024.hdr"
    "material_simple.frag"
    "snowflake.png"
)

qt_internal_add_resource(manual_test_reflectionprobe "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
