# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_manual_test(manual_test_shadowCasting
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
)

qt_internal_add_resource(manual_test_shadowCasting "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

