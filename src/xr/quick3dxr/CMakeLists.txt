# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(Quick3DXr
    URI "QtQuick3D.Xr"
    VERSION "${PROJECT_VERSION}"
    DEPENDENCIES QtQuick QtQuick3D
    SOURCES
        qopenxrhandtrackerinput_p.h qopenxrhandtrackerinput.cpp
        qopenxractionmapper_p.h qopenxractionmapper.cpp
        qabstractopenxrgraphics.cpp qabstractopenxrgraphics_p.h
        qopenxrorigin.cpp qopenxrorigin_p.h
        qopenxranimationdriver.cpp qopenxranimationdriver_p.h
        qopenxrcamera.cpp qopenxrcamera_p.h
        qopenxrhelpers.cpp qopenxrhelpers_p.h
        qopenxrmanager.cpp qopenxrmanager_p.h
        qopenxrplatform_p.h
        qopenxrview.cpp qopenxrview_p.h
        qopenxrinputmanager.cpp qopenxrinputmanager_p.h
        qopenxrhandinput.cpp qopenxrhandinput_p.h
        qopenxrgamepadinput.cpp qopenxrgamepadinput_p.h
        qopenxrvirtualmouse.cpp qopenxrvirtualmouse_p.h
        qopenxrcontroller_p.h qopenxrcontroller.cpp
        qopenxrspatialanchor_p.h qopenxrspatialanchor.cpp
        qopenxrspatialanchormodel_p.h qopenxrspatialanchormodel.cpp
        qopenxrspaceextension_p.h qopenxrspaceextension.cpp
        qopenxrruntimeinfo_p.h qopenxrruntimeinfo.cpp
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
    GENERATE_CPP_EXPORTS
)

# OpenXR Dependency

qt_find_package(WrapOpenXR PROVIDED_TARGETS WrapOpenXR::WrapOpenXR)
target_link_libraries(Quick3DXr PUBLIC WrapOpenXR::WrapOpenXR)

# Backends

# Win32 / DX11
qt_internal_extend_target(Quick3DXr CONDITION WIN32
    SOURCES
        qopenxrgraphics_d3d11.cpp qopenxrgraphics_d3d11_p.h
        qopenxrgraphics_d3d12.cpp qopenxrgraphics_d3d12_p.h
    DEFINES
        XR_USE_PLATFORM_WIN32
        XR_USE_GRAPHICS_API_D3D11
        XR_USE_GRAPHICS_API_D3D12
    LIBRARIES
        user32
)

# Linux / xcb
qt_internal_extend_target(Quick3DXr CONDITION QT_FEATURE_xcb AND QT_FEATURE_xcb_glx_plugin
    DEFINES
        XR_USE_PLATFORM_XCB
)

# OpenGL

qt_internal_extend_target(Quick3DXr CONDITION QT_FEATURE_opengl AND NOT QT_FEATURE_opengles2 AND NOT MACOS
    SOURCES
        qopenxrgraphics_opengl.cpp qopenxrgraphics_opengl_p.h
    DEFINES
        XR_USE_GRAPHICS_API_OPENGL
)

qt_internal_extend_target(Quick3DXr CONDITION ANDROID AND QT_FEATURE_opengles2
    SOURCES
        qopenxrgraphics_opengles.cpp qopenxrgraphics_opengles_p.h
    DEFINES
        XR_USE_GRAPHICS_API_OPENGL_ES
)

# Vulkan
qt_internal_extend_target(Quick3DXr CONDITION QT_FEATURE_vulkan
    SOURCES
        qopenxrgraphics_vulkan.cpp qopenxrgraphics_vulkan_p.h
    DEFINES
        XR_USE_GRAPHICS_API_VULKAN
)

qt_internal_extend_target(Quick3DXr CONDITION ANDROID
     DEFINES
        XR_USE_PLATFORM_ANDROID
)
