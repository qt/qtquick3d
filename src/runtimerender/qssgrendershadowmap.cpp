// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderShadowMap::QSSGRenderShadowMap(const QSSGRenderContextInterface &inContext)
    : m_context(inContext)
{
}

QSSGRenderShadowMap::~QSSGRenderShadowMap()
{
    releaseCachedResources();
}

void QSSGRenderShadowMap::releaseCachedResources()
{
    for (QSSGShadowMapEntry &entry : m_shadowMapList)
        entry.destroyRhiResources();

    m_shadowMapList.clear();
}

static QRhiTexture *allocateRhiShadowTexture(QRhi *rhi,
                                             QRhiTexture::Format format,
                                             const QSize &size,
                                             QRhiTexture::Flags flags = {})
{
    auto texture = rhi->newTexture(format, size, 1, flags);
    if (!texture->create())
        qWarning("Failed to create shadow map texture of size %dx%d", size.width(), size.height());
    return texture;
}

static QRhiRenderBuffer *allocateRhiShadowRenderBuffer(QRhi *rhi,
                                                       QRhiRenderBuffer::Type type,
                                                       const QSize &size)
{
    auto renderBuffer = rhi->newRenderBuffer(type, size, 1);
    if (!renderBuffer->create())
        qWarning("Failed to build depth-stencil buffer of size %dx%d", size.width(), size.height());
    return renderBuffer;
}

static inline void setupForRhiDepthCube(QRhi *rhi,
                                        QSSGShadowMapEntry *entry,
                                        const QSize &size,
                                        QRhiTexture::Format format)
{
    entry->m_rhiDepthCube = allocateRhiShadowTexture(rhi, format, size, QRhiTexture::RenderTarget | QRhiTexture::CubeMap);
    entry->m_rhiCubeCopy = allocateRhiShadowTexture(rhi, format, size, QRhiTexture::RenderTarget | QRhiTexture::CubeMap);
    entry->m_rhiDepthStencil = allocateRhiShadowRenderBuffer(rhi, QRhiRenderBuffer::DepthStencil, size);
}

static inline void setupForRhiDepth(QRhi *rhi,
                                    QSSGShadowMapEntry *entry,
                                    const QSize &size,
                                    QRhiTexture::Format format)
{
    entry->m_rhiDepthMap = allocateRhiShadowTexture(rhi, format, size, QRhiTexture::RenderTarget);
    entry->m_rhiDepthCopy = allocateRhiShadowTexture(rhi, format, size, QRhiTexture::RenderTarget);
    entry->m_rhiDepthStencil = allocateRhiShadowRenderBuffer(rhi, QRhiRenderBuffer::DepthStencil, size);
}

void QSSGRenderShadowMap::addShadowMapEntry(qint32 lightIdx,
                                            qint32 width,
                                            qint32 height,
                                            ShadowMapModes mode,
                                            const QString &renderNodeObjName)
{
    QRhi *rhi = m_context.rhiContext()->rhi();
    // Bail out if there is no QRhi, since we can't add entries without it
    if (!rhi)
        return;


    const QSize pixelSize(width, height);

    QRhiTexture::Format rhiFormat = QRhiTexture::R16F;
    if (!rhi->isTextureFormatSupported(rhiFormat))
        rhiFormat = QRhiTexture::R16;

    const QByteArray rtName = renderNodeObjName.toLatin1();

    // This function is called once per shadow casting light on every layer
    // prepare (i.e. once per frame). We must avoid creating resources as much
    // as possible: if the shadow mode, dimensions, etc. are all the same as in
    // the previous prepare round, then reuse the existing resources.

    QSSGShadowMapEntry *pEntry = shadowMapEntry(lightIdx);
    if (pEntry) {
        if (pEntry->m_rhiDepthMap && mode == ShadowMapModes::CUBE) {
            // previously VSM now CUBE
            pEntry->destroyRhiResources();
            setupForRhiDepthCube(rhi, pEntry, pixelSize, rhiFormat);
        } else if (pEntry->m_rhiDepthCube && mode != ShadowMapModes::CUBE) {
            // previously CUBE now VSM
            pEntry->destroyRhiResources();
            setupForRhiDepth(rhi, pEntry, pixelSize, rhiFormat);
        } else if (pEntry->m_rhiDepthMap) {
            // VSM before and now, see if size has changed
            if (pEntry->m_rhiDepthMap->pixelSize() != pixelSize) {
                pEntry->destroyRhiResources();
                setupForRhiDepth(rhi, pEntry, pixelSize, rhiFormat);
            }
        } else if (pEntry->m_rhiDepthCube) {
            // CUBE before and now, see if size has changed
            if (pEntry->m_rhiDepthCube->pixelSize() != pixelSize) {
                pEntry->destroyRhiResources();
                setupForRhiDepthCube(rhi, pEntry, pixelSize, rhiFormat);
            }
        }
        pEntry->m_shadowMapMode = mode;
    } else if (mode == ShadowMapModes::CUBE) {
        QRhiTexture *depthMap = allocateRhiShadowTexture(rhi, rhiFormat, pixelSize, QRhiTexture::RenderTarget | QRhiTexture::CubeMap);
        QRhiTexture *depthCopy = allocateRhiShadowTexture(rhi, rhiFormat, pixelSize, QRhiTexture::RenderTarget | QRhiTexture::CubeMap);
        QRhiRenderBuffer *depthStencil = allocateRhiShadowRenderBuffer(rhi, QRhiRenderBuffer::DepthStencil, pixelSize);
        m_shadowMapList.push_back(QSSGShadowMapEntry::withRhiDepthCubeMap(lightIdx, mode, depthMap, depthCopy, depthStencil));

        pEntry = &m_shadowMapList.back();
    } else { // VSM
        Q_ASSERT(mode == ShadowMapModes::VSM);
        QRhiTexture *depthMap = allocateRhiShadowTexture(rhi, rhiFormat, QSize(width, height), QRhiTexture::RenderTarget);
        QRhiTexture *depthCopy = allocateRhiShadowTexture(rhi, rhiFormat, QSize(width, height), QRhiTexture::RenderTarget);
        QRhiRenderBuffer *depthStencil = allocateRhiShadowRenderBuffer(rhi, QRhiRenderBuffer::DepthStencil, pixelSize);
        m_shadowMapList.push_back(QSSGShadowMapEntry::withRhiDepthMap(lightIdx, mode, depthMap, depthCopy, depthStencil));

        pEntry = &m_shadowMapList.back();
    }

    if (pEntry) {
        // Additional graphics resources: samplers, render targets.
        if (mode == ShadowMapModes::VSM) {
            if (pEntry->m_rhiRenderTargets.isEmpty()) {
                pEntry->m_rhiRenderTargets.resize(1);
                pEntry->m_rhiRenderTargets[0] = nullptr;
            }
            Q_ASSERT(pEntry->m_rhiRenderTargets.size() == 1);

            QRhiTextureRenderTarget *&rt(pEntry->m_rhiRenderTargets[0]);
            if (!rt) {
                QRhiTextureRenderTargetDescription rtDesc;
                rtDesc.setColorAttachments({ pEntry->m_rhiDepthMap });
                rtDesc.setDepthStencilBuffer(pEntry->m_rhiDepthStencil);
                rt = rhi->newTextureRenderTarget(rtDesc);
                rt->setDescription(rtDesc);
                // The same renderpass descriptor can be reused since the
                // format, load/store ops are the same regardless of the shadow mode.
                if (!pEntry->m_rhiRenderPassDesc)
                    pEntry->m_rhiRenderPassDesc = rt->newCompatibleRenderPassDescriptor();
                rt->setRenderPassDescriptor(pEntry->m_rhiRenderPassDesc);
                if (!rt->create())
                    qWarning("Failed to build shadow map render target");
            }
            rt->setName(rtName + QByteArrayLiteral(" shadow map"));

            if (!pEntry->m_rhiBlurRenderTarget0) {
                // blur X: depthMap -> depthCopy
                pEntry->m_rhiBlurRenderTarget0 = rhi->newTextureRenderTarget({ pEntry->m_rhiDepthCopy });
                if (!pEntry->m_rhiBlurRenderPassDesc)
                    pEntry->m_rhiBlurRenderPassDesc = pEntry->m_rhiBlurRenderTarget0->newCompatibleRenderPassDescriptor();
                pEntry->m_rhiBlurRenderTarget0->setRenderPassDescriptor(pEntry->m_rhiBlurRenderPassDesc);
                pEntry->m_rhiBlurRenderTarget0->create();
            }
            pEntry->m_rhiBlurRenderTarget0->setName(rtName + QByteArrayLiteral(" shadow blur X"));
            if (!pEntry->m_rhiBlurRenderTarget1) {
                // blur Y: depthCopy -> depthMap
                pEntry->m_rhiBlurRenderTarget1 = rhi->newTextureRenderTarget({ pEntry->m_rhiDepthMap });
                pEntry->m_rhiBlurRenderTarget1->setRenderPassDescriptor(pEntry->m_rhiBlurRenderPassDesc);
                pEntry->m_rhiBlurRenderTarget1->create();
            }
            pEntry->m_rhiBlurRenderTarget1->setName(rtName + QByteArrayLiteral(" shadow blur Y"));
        } else {
            if (pEntry->m_rhiRenderTargets.isEmpty()) {
                pEntry->m_rhiRenderTargets.resize(6);
                for (int i = 0; i < 6; ++i)
                    pEntry->m_rhiRenderTargets[i] = nullptr;
            }
            Q_ASSERT(pEntry->m_rhiRenderTargets.size() == 6);

            for (const auto face : QSSGRenderTextureCubeFaces) {
                QRhiTextureRenderTarget *&rt(pEntry->m_rhiRenderTargets[quint8(face)]);
                if (!rt) {
                    QRhiColorAttachment att(pEntry->m_rhiDepthCube);
                    att.setLayer(quint8(face)); // 6 render targets, each referencing one face of the cubemap
                    QRhiTextureRenderTargetDescription rtDesc;
                    rtDesc.setColorAttachments({ att });
                    rtDesc.setDepthStencilBuffer(pEntry->m_rhiDepthStencil);
                    rt = rhi->newTextureRenderTarget(rtDesc);
                    rt->setDescription(rtDesc);
                    if (!pEntry->m_rhiRenderPassDesc)
                        pEntry->m_rhiRenderPassDesc = rt->newCompatibleRenderPassDescriptor();
                    rt->setRenderPassDescriptor(pEntry->m_rhiRenderPassDesc);
                    if (!rt->create())
                        qWarning("Failed to build shadow map render target");
                }
                rt->setName(rtName + QByteArrayLiteral(" shadow cube face: ") + QSSGBaseTypeHelpers::displayName(face));
            }

            // blurring cubemap happens via multiple render targets (all faces attached to COLOR0..5)
            if (rhi->resourceLimit(QRhi::MaxColorAttachments) >= 6) {
                if (!pEntry->m_rhiBlurRenderTarget0) {
                    // blur X: depthCube -> cubeCopy
                    QRhiColorAttachment att[6];
                    for (const auto face : QSSGRenderTextureCubeFaces) {
                        att[quint8(face)].setTexture(pEntry->m_rhiCubeCopy);
                        att[quint8(face)].setLayer(quint8(face));
                    }
                    QRhiTextureRenderTargetDescription rtDesc;
                    rtDesc.setColorAttachments(att, att + 6);
                    pEntry->m_rhiBlurRenderTarget0 = rhi->newTextureRenderTarget(rtDesc);
                    if (!pEntry->m_rhiBlurRenderPassDesc)
                        pEntry->m_rhiBlurRenderPassDesc = pEntry->m_rhiBlurRenderTarget0->newCompatibleRenderPassDescriptor();
                    pEntry->m_rhiBlurRenderTarget0->setRenderPassDescriptor(pEntry->m_rhiBlurRenderPassDesc);
                    pEntry->m_rhiBlurRenderTarget0->create();
                }
                pEntry->m_rhiBlurRenderTarget0->setName(rtName + QByteArrayLiteral(" shadow cube blur X"));
                if (!pEntry->m_rhiBlurRenderTarget1) {
                    // blur Y: cubeCopy -> depthCube
                    QRhiColorAttachment att[6];
                    for (const auto face : QSSGRenderTextureCubeFaces) {
                        att[quint8(face)].setTexture(pEntry->m_rhiDepthCube);
                        att[quint8(face)].setLayer(quint8(face));
                    }
                    QRhiTextureRenderTargetDescription rtDesc;
                    rtDesc.setColorAttachments(att, att + 6);
                    pEntry->m_rhiBlurRenderTarget1 = rhi->newTextureRenderTarget(rtDesc);
                    pEntry->m_rhiBlurRenderTarget1->setRenderPassDescriptor(pEntry->m_rhiBlurRenderPassDesc);
                    pEntry->m_rhiBlurRenderTarget1->create();
                }
                pEntry->m_rhiBlurRenderTarget1->setName(rtName + QByteArrayLiteral(" shadow cube blur Y"));
            } else {
                static bool warned = false;
                if (!warned) {
                    warned = true;
                    qWarning("Cubemap-based shadow maps will not be blurred because MaxColorAttachments is less than 6");
                }
            }
        }

        pEntry->m_lightIndex = lightIdx;
    }
}

QSSGShadowMapEntry *QSSGRenderShadowMap::shadowMapEntry(int lightIdx)
{
    Q_ASSERT(lightIdx >= 0);

    for (int i = 0; i < m_shadowMapList.size(); i++) {
        QSSGShadowMapEntry *pEntry = &m_shadowMapList[i];
        if (pEntry->m_lightIndex == quint32(lightIdx))
            return pEntry;
    }

    return nullptr;
}

QSSGShadowMapEntry::QSSGShadowMapEntry()
    : m_lightIndex(std::numeric_limits<quint32>::max())
    , m_shadowMapMode(ShadowMapModes::VSM)
{
}

QSSGShadowMapEntry QSSGShadowMapEntry::withRhiDepthMap(quint32 lightIdx,
                                                       ShadowMapModes mode,
                                                       QRhiTexture *depthMap,
                                                       QRhiTexture *depthCopy,
                                                       QRhiRenderBuffer *depthStencil)
{
    QSSGShadowMapEntry e;
    e.m_lightIndex = lightIdx;
    e.m_shadowMapMode = mode;
    e.m_rhiDepthMap = depthMap;
    e.m_rhiDepthCopy = depthCopy;
    e.m_rhiDepthStencil = depthStencil;
    return e;
}

QSSGShadowMapEntry QSSGShadowMapEntry::withRhiDepthCubeMap(quint32 lightIdx,
                                                           ShadowMapModes mode,
                                                           QRhiTexture *depthCube,
                                                           QRhiTexture *cubeCopy,
                                                           QRhiRenderBuffer *depthStencil)
{
    QSSGShadowMapEntry e;
    e.m_lightIndex = lightIdx;
    e.m_shadowMapMode = mode;
    e.m_rhiDepthCube = depthCube;
    e.m_rhiCubeCopy = cubeCopy;
    e.m_rhiDepthStencil = depthStencil;
    return e;
}

void QSSGShadowMapEntry::destroyRhiResources()
{
    delete m_rhiDepthMap;
    m_rhiDepthMap = nullptr;
    delete m_rhiDepthCopy;
    m_rhiDepthCopy = nullptr;
    delete m_rhiDepthCube;
    m_rhiDepthCube = nullptr;
    delete m_rhiCubeCopy;
    m_rhiCubeCopy = nullptr;
    delete m_rhiDepthStencil;
    m_rhiDepthStencil = nullptr;

    qDeleteAll(m_rhiRenderTargets);
    m_rhiRenderTargets.clear();
    delete m_rhiRenderPassDesc;
    m_rhiRenderPassDesc = nullptr;
    delete m_rhiBlurRenderTarget0;
    m_rhiBlurRenderTarget0 = nullptr;
    delete m_rhiBlurRenderTarget1;
    m_rhiBlurRenderTarget1 = nullptr;
    delete m_rhiBlurRenderPassDesc;
    m_rhiBlurRenderPassDesc = nullptr;
}

QT_END_NAMESPACE
