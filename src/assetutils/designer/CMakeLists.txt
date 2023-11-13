# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/AssetUtils/designer")
qt_copy_or_install(
    FILES
        RuntimeLoaderSection.qml
        RuntimeLoaderSpecifics.qml
        NodeSection.qml
        assetutils.metainfo
    DESTINATION "${destination}"
)

add_subdirectory(images)
