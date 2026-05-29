# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Quick3DPrivate ShaderTools)

qt_add_library(VFExtension)

qt_add_qml_module(VFExtension
    URI VFExtension
    VERSION 1.0
    QML_FILES
        VolumetricFogEffect.qml
    SOURCES
        volumetricfogextension.cpp
        volumetricfogextension.h
        ieslightprofileindex.cpp
        ieslightprofileindex.h
        iestexturedata.cpp
        iestexturedata.h
        fog3dvolume.cpp
        fog3dvolume.h
    RESOURCES
        shaders/volumetricLight.frag
)

qt_add_shaders(VFExtension "froxel_shaders"
    GLSL "430"
    PREFIX ""
    FILES
        shaders/froxellightinjection.comp
)

target_link_libraries(VFExtension
    PRIVATE
    Qt6::Quick
    Qt6::Quick3D
    Qt6::Quick3DPrivate
)
