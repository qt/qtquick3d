# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

set_source_files_properties(Constants.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
)

qt_add_library(Constants STATIC)
qt_add_qml_module(Constants
    URI "VirtualAssistant.Constants"
    VERSION 1.0
    QML_FILES
        Constants.qml
)

target_link_libraries(Constants
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Quick
)
