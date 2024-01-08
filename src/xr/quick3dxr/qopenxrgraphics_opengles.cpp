// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgraphics_opengles_p.h"
#include "qopenxrhelpers_p.h"

#include <QtGui/private/qrhigles2_p.h>
#include <QtGui/private/qeglplatformcontext_p.h>
#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

QOpenXRGraphicsOpenGLES::QOpenXRGraphicsOpenGLES()
{
#ifdef XR_USE_PLATFORM_ANDROID
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
#endif
    m_graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
}


bool QOpenXRGraphicsOpenGLES::isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const
{
    for (const auto &extension : extensions) {
        if (!strcmp(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
                    extension.extensionName))
            return true;
    }
    return false;
}


const char *QOpenXRGraphicsOpenGLES::extensionName() const
{
    return XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME;
}


const XrBaseInStructure *QOpenXRGraphicsOpenGLES::handle() const
{
    return reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
}


bool QOpenXRGraphicsOpenGLES::setupGraphics(const XrInstance &instance, XrSystemId &systemId, const QQuickGraphicsConfiguration &)
{
    // Extension function must be loaded by name
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
    OpenXRHelpers::checkXrResult(xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                                                       reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLESGraphicsRequirementsKHR)),
                                 instance);

    OpenXRHelpers::checkXrResult(pfnGetOpenGLESGraphicsRequirementsKHR(instance, systemId, &m_graphicsRequirements),
                                 instance);
    return true;
}

bool QOpenXRGraphicsOpenGLES::finializeGraphics(QRhi *rhi)
{
    const QRhiGles2NativeHandles *openglRhi = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());

    auto context = openglRhi->context;
    const XrVersion desiredApiVersion = XR_MAKE_VERSION(context->format().majorVersion(), context->format().minorVersion(), 0);
    if (m_graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
        qDebug() << "Runtime does not support desired Graphics API and/or version";
        return false;
    }

#ifdef XR_USE_PLATFORM_ANDROID
    QEGLPlatformContext *eglPlatformContext = static_cast<QEGLPlatformContext *>(context->handle());
    m_graphicsBinding.display = eglPlatformContext->eglDisplay();
    m_graphicsBinding.config = eglPlatformContext->eglConfig();
    m_graphicsBinding.context = eglPlatformContext->eglContext();
#endif

    return true;
}


int64_t QOpenXRGraphicsOpenGLES::colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const
{
    // List of supported color swapchain formats.
    constexpr int64_t SupportedColorSwapchainFormats[] = {
        GL_SRGB8_ALPHA8_EXT,
        GL_RGBA8_SNORM
    };
    auto swapchainFormatIt = std::find_first_of(swapchainFormats.begin(),
                                                swapchainFormats.end(),
                                                std::begin(SupportedColorSwapchainFormats),
                                                std::end(SupportedColorSwapchainFormats));

    return *swapchainFormatIt;
}


QVector<XrSwapchainImageBaseHeader*> QOpenXRGraphicsOpenGLES::allocateSwapchainImages(int count, XrSwapchain swapchain)
{
    QVector<XrSwapchainImageBaseHeader*> swapchainImages;
    QVector<XrSwapchainImageOpenGLESKHR> swapchainImageBuffer(count);
    for (XrSwapchainImageOpenGLESKHR& image : swapchainImageBuffer) {
        image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        swapchainImages.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
    }
    m_swapchainImageBuffer.insert(swapchain, swapchainImageBuffer);
    return swapchainImages;
}


QQuickRenderTarget QOpenXRGraphicsOpenGLES::renderTarget(const XrCompositionLayerProjectionView &layerView, const XrSwapchainImageBaseHeader *swapchainImage, quint64 swapchainFormat, int arraySize) const
{
    const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLESKHR*>(swapchainImage)->image;
    if (arraySize <= 1) {
        return QQuickRenderTarget::fromOpenGLTexture(colorTexture,
                                                     QSize(layerView.subImage.imageRect.extent.width,
                                                           layerView.subImage.imageRect.extent.height));
    } else {
        return QQuickRenderTarget::fromOpenGLTextureMultiView(colorTexture,
                                                              swapchainFormat,
                                                              QSize(layerView.subImage.imageRect.extent.width,
                                                                    layerView.subImage.imageRect.extent.height),
                                                              1,
                                                              arraySize);
    }
}

QT_END_NAMESPACE
