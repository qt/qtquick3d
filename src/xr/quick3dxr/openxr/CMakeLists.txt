qt_find_package(WrapOpenXR PROVIDED_TARGETS WrapOpenXR::WrapOpenXR)
target_link_libraries(Quick3DXr PUBLIC WrapOpenXR::WrapOpenXR)

qt_internal_extend_target(Quick3DXr
    SOURCES
        qopenxrplatform_p.h
        qquick3dxrmanager_openxr.cpp qquick3dxrmanager_openxr_p.h
        qabstractopenxrgraphics.cpp qabstractopenxrgraphics_p.h
        qopenxrinputmanager.cpp qopenxrinputmanager_p.h
        qquick3dxranchormanager_openxr.cpp qquick3dxranchormanager_openxr_p.h
        qopenxrhelpers.cpp qopenxrhelpers_p.h
)

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
