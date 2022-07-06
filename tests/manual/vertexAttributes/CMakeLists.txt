# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_manual_test(manual_test_vertexAttributes
    GUI
    SOURCES
        main.cpp
        testgeometry.cpp testgeometry.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

qt_internal_add_resource(manual_test_vertexAttributes "qml"
    PREFIX
        "/"
    FILES
        main.qml
        Exhibit.qml
)

qt_internal_add_resource(manual_test_vertexAttributes "maps"
    PREFIX
        "/"
    FILES
        maps/basemetal_small.png
        maps/normalrough_small.png
)
