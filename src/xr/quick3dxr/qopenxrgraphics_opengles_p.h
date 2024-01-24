// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRGRAPHICSOPENGLES_H
#define QOPENXRGRAPHICSOPENGLES_H

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

class QOpenXRGraphicsOpenGLES : public QAbstractOpenXRGraphics
{
public:
    QOpenXRGraphicsOpenGLES();

    bool isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const override;
    const char *extensionName() const override;
    const XrBaseInStructure *handle() const override;
    bool setupGraphics(const XrInstance &instance, XrSystemId &systemId, const QQuickGraphicsConfiguration &quickConfig) override;
    bool finializeGraphics(QRhi *rhi) override;
    int64_t colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const override;
    QVector<XrSwapchainImageBaseHeader*> allocateSwapchainImages(int count, XrSwapchain swapchain) override;
    QQuickRenderTarget renderTarget(const XrSwapchainSubImage &subImage, const XrSwapchainImageBaseHeader *swapchainImage, quint64 swapchainFormat, int arraySize) const override;

private:
#ifdef XR_USE_PLATFORM_ANDROID
    XrGraphicsBindingOpenGLESAndroidKHR m_graphicsBinding{};
#endif
    QMap<XrSwapchain, QVector<XrSwapchainImageOpenGLESKHR>> m_swapchainImageBuffer;

    XrGraphicsRequirementsOpenGLESKHR m_graphicsRequirements{};
};

QT_END_NAMESPACE

#endif // QOPENXRGRAPHICSOPENGLES_H
