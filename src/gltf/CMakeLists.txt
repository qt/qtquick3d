# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(Quick3DGltfPrivate
    INTERNAL_MODULE
    SOURCES
        qssggltfaccessorreader.cpp qssggltfaccessorreader_p.h
        qssggltfdocument_p.h
        qssggltfparser.cpp qssggltfparser_p.h
        qssggltfresourceresolver.cpp qssggltfresourceresolver_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)
