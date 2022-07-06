# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/designer/source")
qt_copy_or_install(
    FILES
        custom_material_default_shader.frag
        effect_default_shader.frag
        effect_template.qml
        view3D_template.qml
    DESTINATION "${destination}"
)
