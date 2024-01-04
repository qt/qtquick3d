// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgraphics_opengl_p.h"
#include "qopenxrhelpers_p.h"

#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickWindow>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

QOpenXRGraphicsOpenGL::QOpenXRGraphicsOpenGL()
{
#ifdef XR_USE_PLATFORM_WIN32
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
#elif defined(XR_USE_PLATFORM_XLIB)
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
#elif defined(XR_USE_PLATFORM_XCB)
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR;
#elif defined(XR_USE_PLATFORM_WAYLAND)
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR;
#endif

    m_graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
}


bool QOpenXRGraphicsOpenGL::isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const
{
    for (const auto &extension : extensions) {
        if (!strcmp(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
                    extension.extensionName))
            return true;
    }
    return false;
}


const char *QOpenXRGraphicsOpenGL::extensionName() const
{
    return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
}


const XrBaseInStructure *QOpenXRGraphicsOpenGL::handle() const
{
    return reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
}


bool QOpenXRGraphicsOpenGL::setupGraphics(const XrInstance &instance, XrSystemId &systemId)
{
    // Extension function must be loaded by name
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    OpenXRHelpers::checkXrResult(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
                                                       reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)),
                                 instance);
    OpenXRHelpers::checkXrResult(pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &m_graphicsRequirements),
                                 instance);
    return true;
}

bool QOpenXRGraphicsOpenGL::finializeGraphics(QRhi *rhi)
{
    const QRhiGles2NativeHandles *openglRhi = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());

    auto context = openglRhi->context;

    const XrVersion desiredApiVersion = XR_MAKE_VERSION(context->format().majorVersion(), context->format().minorVersion(), 0);
    if (m_graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
        qDebug() << "Runtime does not support desired Graphics API and/or version";
        return false;
    }

# ifdef XR_USE_PLATFORM_WIN32
    auto nativeContext = context->nativeInterface<QNativeInterface::QWGLContext>();
    if (nativeContext) {
        m_graphicsBinding.hGLRC = nativeContext->nativeContext();
        m_graphicsBinding.hDC = GetDC(reinterpret_cast<HWND>(m_window->winId()));
    }
# endif

    return true;
}


int64_t QOpenXRGraphicsOpenGL::colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const
{
    // List of supported color swapchain formats.
    constexpr int64_t SupportedColorSwapchainFormats[] = {
        GL_RGBA8,
        GL_RGBA8_SNORM,
    };
    auto swapchainFormatIt = std::find_first_of(swapchainFormats.begin(),
                                                swapchainFormats.end(),
                                                std::begin(SupportedColorSwapchainFormats),
                                                std::end(SupportedColorSwapchainFormats));

    return *swapchainFormatIt;
}


QVector<XrSwapchainImageBaseHeader*> QOpenXRGraphicsOpenGL::allocateSwapchainImages(int count, XrSwapchain swapchain)
{
    QVector<XrSwapchainImageBaseHeader*> swapchainImages;
    QVector<XrSwapchainImageOpenGLKHR> swapchainImageBuffer(count);
    for (XrSwapchainImageOpenGLKHR& image : swapchainImageBuffer) {
        image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
        swapchainImages.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
    }
    m_swapchainImageBuffer.insert(swapchain, swapchainImageBuffer);
    return swapchainImages;
}


QQuickRenderTarget QOpenXRGraphicsOpenGL::renderTarget(const XrCompositionLayerProjectionView &layerView, const XrSwapchainImageBaseHeader *swapchainImage, quint64 swapchainFormat) const
{
    Q_UNUSED(swapchainFormat)
    const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(swapchainImage)->image;
    return QQuickRenderTarget::fromOpenGLTexture(colorTexture,
                                                 QSize(layerView.subImage.imageRect.extent.width,
                                                       layerView.subImage.imageRect.extent.height));
}

void QOpenXRGraphicsOpenGL::setupWindow(QQuickWindow *window)
{
    m_window = window;
}

QT_END_NAMESPACE
