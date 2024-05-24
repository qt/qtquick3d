// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRPLATFORM_H
#define QOPENXRPLATFORM_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#ifdef XR_USE_GRAPHICS_API_VULKAN
# include <QtGui/QVulkanInstance>
#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
# include <d3d11.h>
#endif

#ifdef XR_USE_GRAPHICS_API_D3D12
# include <d3d12.h>
#endif

#ifdef XR_USE_GRAPHICS_API_OPENGL
# include <QtGui/QOpenGLContext>
#endif


#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
# include <QtGui/QOpenGLContext>
# include <EGL/egl.h>
#endif

#ifdef XR_USE_PLATFORM_ANDROID
# include <QtCore/qnativeinterface.h>
# include <QtCore/QJniEnvironment>
#endif

#ifdef XR_USE_PLATFORM_XCB
# include <xcb/xcb.h>
# include <xcb/glx.h>
#endif

#include <openxr/openxr_platform.h>

#endif // QOPENXRPLATFORM_H
