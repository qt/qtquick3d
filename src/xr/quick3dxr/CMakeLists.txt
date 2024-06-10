# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set_source_files_properties(XrErrorDialog.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "6.8"
)

qt_internal_add_qml_module(Quick3DXr
    URI "QtQuick3D.Xr"
    VERSION "${PROJECT_VERSION}"
    DEPENDENCIES QtQuick QtQuick3D
    SOURCES
        qopenxractionmapper_p.h qopenxractionmapper.cpp
        qquick3dxrorigin.cpp qquick3dxrorigin_p.h
        qquick3dxranimationdriver.cpp qquick3dxranimationdriver_p.h
        qquick3dxrcamera.cpp qquick3dxrcamera_p.h
        qquick3dxrmanager.cpp qquick3dxrmanager_p.h
        qquick3dxrview.cpp qquick3dxrview_p.h
        qquick3dxrruntimeinfo_p.h qquick3dxrruntimeinfo.cpp
        qquick3dxritem_p.h qquick3dxritem.cpp
        qquick3dxrvirtualmouse.cpp qquick3dxrvirtualmouse_p.h
        qopenxrcontroller_p.h qopenxrcontroller.cpp
        qquick3dxrhandinput.cpp qquick3dxrhandinput_p.h qquick3dxrhandtrackerinput_p_p.h
        qquick3dxrgamepadinput.cpp qquick3dxrgamepadinput_p.h
        qquick3dxrhandtrackerinput_p.h qquick3dxrhandtrackerinput.cpp
        qquick3dxrinputmanager.cpp qquick3dxrinputmanager_p.h
        qtquick3dxrglobal_p.h
        qtquick3dxrglobal.h
    QML_FILES
        XrErrorDialog.qml
    DEFINES
        QT_BUILD_QUICK3DXR_LIB
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick3DPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Quick3D
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick3D
        Qt::QuickPrivate
)

if (VISIONOS)
    add_subdirectory(visionos)
else ()
    add_subdirectory(openxr)

    # Non-ported/not suitable for VisionOS
    qt_internal_extend_target(Quick3DXr
        SOURCES
            qopenxrspatialanchor_p.h qopenxrspatialanchor.cpp
            qopenxrspatialanchormodel_p.h qopenxrspatialanchormodel.cpp
            qopenxrspaceextension_p.h qopenxrspaceextension.cpp
    )

endif () # VISIONOS
