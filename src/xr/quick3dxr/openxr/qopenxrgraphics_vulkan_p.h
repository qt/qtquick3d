// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRGRAPHICSVULKAN_H
#define QOPENXRGRAPHICSVULKAN_H

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
#include <QtGui/QVulkanInstance>
#include <QtQuick/QQuickGraphicsConfiguration>

QT_BEGIN_NAMESPACE

class QRhiTexture;

class QOpenXRGraphicsVulkan : public QAbstractOpenXRGraphics
{
public:
    QOpenXRGraphicsVulkan();

    const char *extensionName() const override;
    bool isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const override;
    const XrBaseInStructure *handle() const override;
    bool setupGraphics(const XrInstance &instance, XrSystemId &systemId, const QQuickGraphicsConfiguration &quickConfig) override;
    bool finializeGraphics(QRhi *rhi) override;
    int64_t colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const override;
    int64_t depthSwapchainFormat(const QVector<int64_t> &swapchainFormats) const override;
    QVector<XrSwapchainImageBaseHeader*> allocateSwapchainImages(int count, XrSwapchain swapchain) override;
    QQuickRenderTarget renderTarget(const XrSwapchainSubImage &subImage, const XrSwapchainImageBaseHeader *swapchainImage,
                                    quint64 swapchainFormat, int samples, int arraySize,
                                    const XrSwapchainImageBaseHeader *depthSwapchainImage, quint64 depthSwapchainFormat) const override;
    void setupWindow(QQuickWindow *quickWindow) override;
    QRhi *rhi() const override { return m_rhi; }
    void releaseResources() override;

private:
    QVulkanInstance m_vulkanInstance;
    VkDevice m_vulkanDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_vulkanPhysicalDevice = VK_NULL_HANDLE;
    VkQueue m_vulkanCommandQueue = VK_NULL_HANDLE;
    QQuickGraphicsConfiguration m_graphicsConfiguration;
    int m_queueFamilyIndex = -1;
    XrGraphicsBindingVulkanKHR m_graphicsBinding{};
    QMap<XrSwapchain, QVector<XrSwapchainImageVulkanKHR>> m_swapchainImageBuffer;
    QRhi *m_rhi = nullptr;
    mutable QRhiTexture *m_depthTexture = nullptr;
};

QT_END_NAMESPACE

#endif // QOPENXRGRAPHICSVULKAN_H
