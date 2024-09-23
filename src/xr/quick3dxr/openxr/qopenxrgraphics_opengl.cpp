// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgraphics_opengl_p.h"
#include "qopenxrhelpers_p.h"

#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickrendertarget_p.h>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

#ifndef GL_RGBA8
#define GL_RGBA8                          0x8058
#endif

#ifndef GL_SRGB8_ALPHA8_EXT
#define GL_SRGB8_ALPHA8_EXT               0x8C43
#endif

#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16              0x81A5
#endif

#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24              0x81A6
#endif

#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F             0x8CAC
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8               0x88F0
#endif

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


bool QOpenXRGraphicsOpenGL::setupGraphics(const XrInstance &instance, XrSystemId &systemId, const QQuickGraphicsConfiguration &)
{
    // Extension function must be loaded by name
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    OpenXRHelpers::checkXrResult(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
                                                       reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)),
                                 instance);
    if (!pfnGetOpenGLGraphicsRequirementsKHR) {
        qWarning("Could not resolve xrGetOpenGLGraphicsRequirementsKHR; perhaps the OpenXR implementation does not support OpenGL?");
        return false;
    }
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
        qWarning("Qt Quick 3D XR (OpenGL): Runtime does not support desired graphics API and/or version");
        return false;
    }

# ifdef XR_USE_PLATFORM_WIN32
    auto nativeContext = context->nativeInterface<QNativeInterface::QWGLContext>();
    if (nativeContext) {
        m_graphicsBinding.hGLRC = nativeContext->nativeContext();
        m_graphicsBinding.hDC = GetDC(reinterpret_cast<HWND>(m_window->winId()));
    }
# endif

    m_rhi = rhi;

    return true;
}


int64_t QOpenXRGraphicsOpenGL::colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const
{
    // List of supported color swapchain formats.
    constexpr int64_t supportedColorSwapchainFormats[] = {
        GL_RGBA8,
        GL_RGBA8_SNORM
    };

    auto swapchainFormatIt = std::find_first_of(std::begin(supportedColorSwapchainFormats),
                                                std::end(supportedColorSwapchainFormats),
                                                swapchainFormats.begin(),
                                                swapchainFormats.end());
    return *swapchainFormatIt;
}

int64_t QOpenXRGraphicsOpenGL::depthSwapchainFormat(const QVector<int64_t> &swapchainFormats) const
{
    // in order of preference
    constexpr int64_t supportedDepthSwapchainFormats[] = {
        GL_DEPTH24_STENCIL8,
        GL_DEPTH_COMPONENT32F,
        GL_DEPTH_COMPONENT24,
        GL_DEPTH_COMPONENT16
    };

    return *std::find_first_of(std::begin(supportedDepthSwapchainFormats),
                               std::end(supportedDepthSwapchainFormats),
                               swapchainFormats.begin(),
                               swapchainFormats.end());
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


QQuickRenderTarget QOpenXRGraphicsOpenGL::renderTarget(const XrSwapchainSubImage &subImage,
                                                       const XrSwapchainImageBaseHeader *swapchainImage,
                                                       quint64 swapchainFormat,
                                                       int samples,
                                                       int arraySize,
                                                       const XrSwapchainImageBaseHeader *depthSwapchainImage,
                                                       quint64 depthSwapchainFormat) const
{
    const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(swapchainImage)->image;

    switch (swapchainFormat) {
    case GL_SRGB8_ALPHA8_EXT:
        swapchainFormat = GL_RGBA8;
        break;
    default:
        break;
    }

    QQuickRenderTarget::Flags flags;
    if (samples > 1)
        flags |= QQuickRenderTarget::Flag::MultisampleResolve;

    const QSize pixelSize(subImage.imageRect.extent.width, subImage.imageRect.extent.height);
    QQuickRenderTarget rt = QQuickRenderTarget::fromOpenGLTexture(colorTexture,
                                                                  swapchainFormat,
                                                                  pixelSize,
                                                                  samples,
                                                                  arraySize,
                                                                  flags);
    if (depthSwapchainImage) {
        QRhiTexture::Format format = QRhiTexture::D24S8;
        switch (depthSwapchainFormat) {
        case GL_DEPTH_COMPONENT32F:
            format = QRhiTexture::D32F;
            break;
        case GL_DEPTH_COMPONENT24:
            format = QRhiTexture::D24;
            break;
        case GL_DEPTH_COMPONENT16:
            format = QRhiTexture::D16;
            break;
        }
        GLuint depthImage = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(depthSwapchainImage)->image;
        if (m_depthTexture && (m_depthTexture->format() != format || m_depthTexture->pixelSize() != pixelSize || m_depthTexture->arraySize() != arraySize)) {
            delete m_depthTexture;
            m_depthTexture = nullptr;
        }
        if (!m_depthTexture) {
            // this is never multisample, QQuickRt takes care of resolving depth-stencil
            if (arraySize > 1)
                m_depthTexture = m_rhi->newTextureArray(format, arraySize, pixelSize, 1, QRhiTexture::RenderTarget);
            else
                m_depthTexture = m_rhi->newTexture(format, pixelSize, 1, QRhiTexture::RenderTarget);
        }
        m_depthTexture->createFrom({ depthImage, 0 });
        rt.setDepthTexture(m_depthTexture);
    }
    return rt;
}

void QOpenXRGraphicsOpenGL::setupWindow(QQuickWindow *window)
{
    m_window = window;
}

void QOpenXRGraphicsOpenGL::releaseResources()
{
    delete m_depthTexture;
    m_depthTexture = nullptr;
}

QT_END_NAMESPACE
