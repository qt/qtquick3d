# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Handle the conditional finding of either system zlib or qt zlib.
if(NOT QT_FEATURE_system_zlib)
    find_package(Qt6 COMPONENTS ZlibPrivate)
elseif(NOT TARGET WrapZLIB::WrapZLIB)
    qt_find_package(WrapZLIB PROVIDED_TARGETS WrapZLIB::WrapZLIB)
endif()

qt_internal_add_module(Quick3DTextureHelpersPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qssgexrcodecs.cpp qssgexrcodecs_p.h
        qssgexrpiz.cpp
        qssgexrreader.cpp qssgexrreader_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

qt_internal_extend_target(Quick3DTextureHelpersPrivate CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(Quick3DTextureHelpersPrivate CONDITION NOT QT_FEATURE_system_zlib
    LIBRARIES
        Qt6::ZlibPrivate
)
