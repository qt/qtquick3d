# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/AssetUtils/designer/images")
qt_copy_or_install(
    FILES
        dummy.png
        dummy16.png
        dummy@2x.png
        runtimeloader.png
        runtimeloader16.png
        runtimeloader@2x.png
    DESTINATION "${destination}"
)
