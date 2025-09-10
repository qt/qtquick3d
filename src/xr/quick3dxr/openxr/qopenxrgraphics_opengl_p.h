// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRGRAPHICSOPENGL_H
#define QOPENXRGRAPHICSOPENGL_H

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


#include <QtQuick3DXr/private/qabstractopenxrgraphics_p.h>
#include <QtQuick3DXr/private/qopenxrplatform_p.h>

QT_BEGIN_NAMESPACE

class QRhiTexture;

class QOpenXRGraphicsOpenGL : public QAbstractOpenXRGraphics
{
public:
    QOpenXRGraphicsOpenGL();

    bool isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const override;
    const char *extensionName() const override;
    const XrBaseInStructure *handle() const override;
    bool setupGraphics(const XrInstance &instance, XrSystemId &systemId, const QQuickGraphicsConfiguration &quickConfig) override;
    bool finializeGraphics(QRhi *rhi) override;
    int64_t colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const override;
    int64_t depthSwapchainFormat(const QVector<int64_t> &swapchainFormats) const override;
    QVector<XrSwapchainImageBaseHeader*> allocateSwapchainImages(int count, XrSwapchain swapchain) override;
    QQuickRenderTarget renderTarget(const XrSwapchainSubImage &subImage, const XrSwapchainImageBaseHeader *swapchainImage,
                                    quint64 swapchainFormat, int samples, int arraySize,
                                    const XrSwapchainImageBaseHeader *depthSwapchainImage, quint64 depthSwapchainFormat) const override;
    void setupWindow(QQuickWindow *window) override;
    QRhi *rhi() const override { return m_rhi; }
    void releaseResources() override;

private:
#ifdef XR_USE_PLATFORM_WIN32
    XrGraphicsBindingOpenGLWin32KHR m_graphicsBinding{};
#elif defined(XR_USE_PLATFORM_XLIB)
    XrGraphicsBindingOpenGLXlibKHR m_graphicsBinding{};
#elif defined(XR_USE_PLATFORM_XCB)
    XrGraphicsBindingOpenGLXcbKHR m_graphicsBinding{};
#elif defined(XR_USE_PLATFORM_WAYLAND)
    XrGraphicsBindingOpenGLWaylandKHR m_graphicsBinding{};
#else
    XrBaseInStructure m_graphicsBinding{}; // just so that the code compiles
#endif
    QMap<XrSwapchain, QVector<XrSwapchainImageOpenGLKHR>> m_swapchainImageBuffer;

    XrGraphicsRequirementsOpenGLKHR m_graphicsRequirements{};
    QWindow *m_window = nullptr;
    QRhi *m_rhi = nullptr;
    mutable QRhiTexture *m_depthTexture = nullptr;
};

QT_END_NAMESPACE

#endif // QOPENXRGRAPHICSOPENGL_H
