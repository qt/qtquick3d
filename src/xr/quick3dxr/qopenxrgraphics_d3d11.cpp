// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgraphics_d3d11_p.h"

#include "qopenxrhelpers_p.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickGraphicsDevice>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

QOpenXRGraphicsD3D11::QOpenXRGraphicsD3D11()
{
    m_graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
    m_graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;
}

bool QOpenXRGraphicsD3D11::isExtensionSupported(const QVector<XrExtensionProperties> &extensions) const
{
    for (const auto &extension : extensions) {
        if (!strcmp(XR_KHR_D3D11_ENABLE_EXTENSION_NAME,
                    extension.extensionName))
            return true;
    }
    return false;
}


const char *QOpenXRGraphicsD3D11::extensionName() const
{
    return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
}


const XrBaseInStructure *QOpenXRGraphicsD3D11::handle() const
{
    return reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
}


bool QOpenXRGraphicsD3D11::setupGraphics(const XrInstance &instance, XrSystemId &systemId)
{
    PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
    OpenXRHelpers::checkXrResult(xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR",
                                                       reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11GraphicsRequirementsKHR)),
                                 instance);

    // Create the D3D11 device for the adapter associated with the system.
    OpenXRHelpers::checkXrResult(pfnGetD3D11GraphicsRequirementsKHR(instance, systemId, &m_graphicsRequirements),
                                 instance);
    return true;
}

bool QOpenXRGraphicsD3D11::finializeGraphics(QRhi *rhi)
{
    const QRhiD3D11NativeHandles *d3d11Rhi = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    m_graphicsBinding.device = reinterpret_cast<ID3D11Device*>(d3d11Rhi->dev);
    return true;
}


int64_t QOpenXRGraphicsD3D11::colorSwapchainFormat(const QVector<int64_t> &swapchainFormats) const
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


QVector<XrSwapchainImageBaseHeader*> QOpenXRGraphicsD3D11::allocateSwapchainImages(int count, XrSwapchain swapchain)
{
    QVector<XrSwapchainImageBaseHeader*> swapchainImages;
    QVector<XrSwapchainImageD3D11KHR> swapchainImageBuffer(count);
    for (XrSwapchainImageD3D11KHR& image : swapchainImageBuffer) {
        image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
        swapchainImages.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
    }
    m_swapchainImageBuffer.insert(swapchain, swapchainImageBuffer);
    return swapchainImages;
}


QQuickRenderTarget QOpenXRGraphicsD3D11::renderTarget(const XrCompositionLayerProjectionView &layerView, const XrSwapchainImageBaseHeader *swapchainImage, quint64 swapchainFormat) const
{
    Q_UNUSED(swapchainFormat)
    ID3D11Texture2D* const colorTexture = reinterpret_cast<const XrSwapchainImageD3D11KHR*>(swapchainImage)->texture;

    return QQuickRenderTarget::fromD3D11Texture(colorTexture,
                                                QSize(layerView.subImage.imageRect.extent.width,
                                                      layerView.subImage.imageRect.extent.height));

}


void QOpenXRGraphicsD3D11::setupWindow(QQuickWindow *quickWindow)
{
    quickWindow->setGraphicsDevice(QQuickGraphicsDevice::fromAdapter(m_graphicsRequirements.adapterLuid.LowPart,
                                                                     m_graphicsRequirements.adapterLuid.HighPart,
                                                                     m_graphicsRequirements.minFeatureLevel));
}

QT_END_NAMESPACE
