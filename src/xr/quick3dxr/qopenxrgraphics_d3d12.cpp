// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgraphics_d3d12_p.h"

#include "qopenxrhelpers_p.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickGraphicsDevice>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

QOpenXRGraphicsD3D12::QOpenXRGraphicsD3D12()
{
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_D3D12_KHR;
    m_graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR;
}

bool QOpenXRGraphicsD3D12::isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const
{
    for (const auto &extension : extensions) {
        if (!strcmp(XR_KHR_D3D12_ENABLE_EXTENSION_NAME,
                    extension.extensionName))
            return true;
    }
    return false;
}


const char *QOpenXRGraphicsD3D12::extensionName() const
{
    return XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
}


const XrBaseInStructure *QOpenXRGraphicsD3D12::handle() const
{
    return reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
}


bool QOpenXRGraphicsD3D12::setupGraphics(const XrInstance &instance, XrSystemId &systemId, const QQuickGraphicsConfiguration &)
{
    PFN_xrGetD3D12GraphicsRequirementsKHR pfnGetD3D12GraphicsRequirementsKHR = nullptr;
    OpenXRHelpers::checkXrResult(xrGetInstanceProcAddr(instance, "xrGetD3D12GraphicsRequirementsKHR",
                                                       reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D12GraphicsRequirementsKHR)),
                                 instance);

    if (!pfnGetD3D12GraphicsRequirementsKHR) {
        qWarning("Could not resolve xrGetD3D12GraphicsRequirementsKHR; perhaps the OpenXR implementation does not support D3D12?");
        return false;
    }

    OpenXRHelpers::checkXrResult(pfnGetD3D12GraphicsRequirementsKHR(instance, systemId, &m_graphicsRequirements),
                                 instance);
    return true;
}

bool QOpenXRGraphicsD3D12::finializeGraphics(QRhi *rhi)
{
    const QRhiD3D12NativeHandles *d3d12Rhi = static_cast<const QRhiD3D12NativeHandles *>(rhi->nativeHandles());
    m_graphicsBinding.device = reinterpret_cast<ID3D12Device*>(d3d12Rhi->dev);
    m_graphicsBinding.queue = reinterpret_cast<ID3D12CommandQueue*>(d3d12Rhi->commandQueue);
    return true;
}


int64_t QOpenXRGraphicsD3D12::colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const
{
    // List of supported color swapchain formats.
    constexpr DXGI_FORMAT SupportedColorSwapchainFormats[] = {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    };
    auto swapchainFormatIt = std::find_first_of(swapchainFormats.begin(),
                                                swapchainFormats.end(),
                                                std::begin(SupportedColorSwapchainFormats),
                                                std::end(SupportedColorSwapchainFormats));

    return *swapchainFormatIt;
}


QVector<XrSwapchainImageBaseHeader*> QOpenXRGraphicsD3D12::allocateSwapchainImages(int count, XrSwapchain swapchain)
{
    QVector<XrSwapchainImageBaseHeader*> swapchainImages;
    QVector<XrSwapchainImageD3D12KHR> swapchainImageBuffer(count);
    for (XrSwapchainImageD3D12KHR& image : swapchainImageBuffer) {
        image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
        swapchainImages.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
    }
    m_swapchainImageBuffer.insert(swapchain, swapchainImageBuffer);
    return swapchainImages;
}


QQuickRenderTarget QOpenXRGraphicsD3D12::renderTarget(const XrSwapchainSubImage &subImage, const XrSwapchainImageBaseHeader *swapchainImage,
                                                      quint64 swapchainFormat, int samples, int arraySize) const
{
    ID3D12Resource* const colorTexture = reinterpret_cast<const XrSwapchainImageD3D12KHR*>(swapchainImage)->texture;

    switch (swapchainFormat) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        swapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        swapchainFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
    default:
        break;
    }

    if (arraySize <= 1) {
        if (samples > 1) {
            return QQuickRenderTarget::fromD3D12TextureWithMultiSampleResolve(colorTexture,
                                                                              0,
                                                                              swapchainFormat,
                                                                              QSize(subImage.imageRect.extent.width,
                                                                                    subImage.imageRect.extent.height),
                                                                              samples);
        } else {
            return QQuickRenderTarget::fromD3D12Texture(colorTexture,
                                                        0,
                                                        swapchainFormat,
                                                        QSize(subImage.imageRect.extent.width,
                                                              subImage.imageRect.extent.height),
                                                        1);
        }
    } else {
        if (samples > 1) {
            return QQuickRenderTarget::fromD3D12TextureMultiViewWithMultiSampleResolve(colorTexture,
                                                                                       0,
                                                                                       swapchainFormat,
                                                                                       QSize(subImage.imageRect.extent.width,
                                                                                             subImage.imageRect.extent.height),
                                                                                       samples,
                                                                                       arraySize);
        } else {
            return QQuickRenderTarget::fromD3D12TextureMultiView(colorTexture,
                                                                0,
                                                                swapchainFormat,
                                                                QSize(subImage.imageRect.extent.width,
                                                                    subImage.imageRect.extent.height),
                                                                1,
                                                                arraySize);
        }
    }
}


void QOpenXRGraphicsD3D12::setupWindow(QQuickWindow *quickWindow)
{
    quickWindow->setGraphicsDevice(QQuickGraphicsDevice::fromAdapter(m_graphicsRequirements.adapterLuid.LowPart,
                                                                     m_graphicsRequirements.adapterLuid.HighPart,
                                                                     m_graphicsRequirements.minFeatureLevel));
}

QT_END_NAMESPACE
