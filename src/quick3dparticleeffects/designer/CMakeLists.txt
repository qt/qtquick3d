# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_copy_or_install(
    FILES
        particleeffects.metainfo
    DESTINATION "${INSTALL_QMLDIR}/QtQuick3D/ParticleEffects/designer"
)

add_subdirectory(images)
add_subdirectory(source)
