// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_SHADOW_MAP_H
#define QSSG_RENDER_SHADOW_MAP_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRenderContextInterface;

class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiTexture;

enum class ShadowMapModes
{
    VSM, ///< variance shadow mapping
    CUBE, ///< cubemap omnidirectional shadows
};

struct QSSGShadowMapEntry
{
    QSSGShadowMapEntry();

    static QSSGShadowMapEntry withRhiDepthMap(quint32 lightIdx,
                                              ShadowMapModes mode,
                                              QRhiTexture *textureArray,
                                              QRhiTexture *depthCopy,
                                              QRhiRenderBuffer *depthStencil);

    static QSSGShadowMapEntry withRhiDepthCubeMap(quint32 lightIdx,
                                                  ShadowMapModes mode,
                                                  QRhiTexture *depthCube,
                                                  QRhiTexture *cubeCopy,
                                                  QRhiRenderBuffer *depthStencil);
    bool isCompatible(QSize mapSize, quint32 layerIndex, ShadowMapModes mapMode);
    void destroyRhiResources();

    quint32 m_lightIndex; ///< the light index it belongs to
    ShadowMapModes m_shadowMapMode; ///< shadow map method
    quint32 m_depthArrayIndex; ///< shadow map texture array index

    // RHI resources
    QRhiTexture *m_rhiDepthTextureArray = nullptr; // for shadow map (VSM) (not owned)
    QRhiTexture *m_rhiDepthCopy = nullptr; // for blur pass (VSM)
    QRhiTexture *m_rhiDepthCube = nullptr; // shadow cube map (CUBE)
    QRhiTexture *m_rhiCubeCopy = nullptr; // for blur pass (CUBE)
    QRhiRenderBuffer *m_rhiDepthStencil = nullptr; // depth/stencil
    std::array<QRhiTextureRenderTarget *, 6> m_rhiRenderTargets = {}; // texture RT
    QRhiRenderPassDescriptor *m_rhiRenderPassDesc = nullptr; // texture RT renderpass descriptor
    QRhiTextureRenderTarget *m_rhiBlurRenderTarget0 = nullptr; // texture RT for blur X (targets depthCopy or cubeCopy)
    QRhiTextureRenderTarget *m_rhiBlurRenderTarget1 = nullptr; // texture RT for blur Y (targets depthMap or depthCube)
    QRhiRenderPassDescriptor *m_rhiBlurRenderPassDesc = nullptr; // blur needs its own because no depth/stencil

    QMatrix4x4 m_lightVP; ///< light view projection matrix
    QMatrix4x4 m_lightCubeView[6]; ///< light cubemap view matrices
    QMatrix4x4 m_lightView; ///< light view transform
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderShadowMap
{
    Q_DISABLE_COPY(QSSGRenderShadowMap)

public:
    const QSSGRenderContextInterface &m_context;

    explicit QSSGRenderShadowMap(const QSSGRenderContextInterface &inContext);
    ~QSSGRenderShadowMap();
    void releaseCachedResources();
    void addShadowMaps(const QSSGShaderLightList &renderableLights);

    QSSGShadowMapEntry *shadowMapEntry(int lightIdx);

    qsizetype shadowMapEntryCount() { return m_shadowMapList.size(); }

private:
    QSSGShadowMapEntry *addDirectionalShadowMap(qint32 lightIdx, QSize size, quint32 layerIndex, const QString &renderNodeObjName);
    QSSGShadowMapEntry *addCubeShadowMap(qint32 lightIdx, QSize size, const QString &renderNodeObjName);

    QVector<QSSGShadowMapEntry> m_shadowMapList;
    QHash<QSize, QRhiTexture *> m_depthTextureArrays;
};

using QSSGRenderShadowMapPtr = std::shared_ptr<QSSGRenderShadowMap>;

QT_END_NAMESPACE

#endif
