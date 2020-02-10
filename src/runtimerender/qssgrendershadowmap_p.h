/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

struct QSSGLayerRenderData;

enum class ShadowMapModes
{
    VSM, ///< variance shadow mapping
    CUBE, ///< cubemap omnidirectional shadows
};

struct QSSGShadowMapEntry
{
    QSSGShadowMapEntry()
        : m_lightIndex(std::numeric_limits<quint32>::max())
        , m_shadowMapMode(ShadowMapModes::VSM)
    {
    }

    static QSSGShadowMapEntry withGlDepthMap(quint32 index,
                                             ShadowMapModes mode,
                                             const QSSGRef<QSSGRenderTexture2D> &depthMap,
                                             const QSSGRef<QSSGRenderTexture2D> &depthCopy,
                                             const QSSGRef<QSSGRenderTexture2D> &depthTemp)
    {
        QSSGShadowMapEntry e;
        e.m_lightIndex = index;
        e.m_shadowMapMode = mode;
        e.m_depthMap = depthMap;
        e.m_depthCopy = depthCopy;
        e.m_depthCube = nullptr;
        e.m_cubeCopy = nullptr;
        e.m_depthRender = depthTemp;
        return e;
    }

    static QSSGShadowMapEntry withGlDepthCubeMap(quint32 index,
                                                 ShadowMapModes mode,
                                                 const QSSGRef<QSSGRenderTextureCube> &depthCube,
                                                 const QSSGRef<QSSGRenderTextureCube> &cubeTmp,
                                                 const QSSGRef<QSSGRenderTexture2D> &depthTemp)
    {
        QSSGShadowMapEntry e;
        e.m_lightIndex = index;
        e.m_shadowMapMode = mode;
        e.m_depthMap = nullptr;
        e.m_depthCopy = nullptr;
        e.m_depthCube = depthCube;
        e.m_cubeCopy = cubeTmp;
        e.m_depthRender = depthTemp;
        return e;
    }

    static QSSGShadowMapEntry withRhiDepthMap(quint32 index,
                                              ShadowMapModes mode,
                                              QRhiTexture *depthMap,
                                              QRhiTexture *depthCopy,
                                              QRhiRenderBuffer *depthStencil)
    {
        QSSGShadowMapEntry e;
        e.m_lightIndex = index;
        e.m_shadowMapMode = mode;
        e.m_rhiDepthMap = depthMap;
        e.m_rhiDepthCopy = depthCopy;
        e.m_rhiDepthStencil = depthStencil;
        return e;
    }

    static QSSGShadowMapEntry withRhiDepthCubeMap(quint32 index,
                                                  ShadowMapModes mode,
                                                  QRhiTexture *depthCube,
                                                  QRhiTexture *cubeCopy,
                                                  QRhiRenderBuffer *depthStencil)
    {
        QSSGShadowMapEntry e;
        e.m_lightIndex = index;
        e.m_shadowMapMode = mode;
        e.m_rhiDepthCube = depthCube;
        e.m_rhiCubeCopy = cubeCopy;
        e.m_rhiDepthStencil = depthStencil;
        return e;
    }

    void destroyRhiResources() {
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

    quint32 m_lightIndex; ///< the light index it belongs to
    ShadowMapModes m_shadowMapMode; ///< shadow map method

    // OpenGL resources
    QSSGRef<QSSGRenderTexture2D> m_depthMap; ///< shadow map texture
    QSSGRef<QSSGRenderTexture2D> m_depthCopy; ///< shadow map buffer used during blur passes
    QSSGRef<QSSGRenderTextureCube> m_depthCube; ///< shadow cube map
    QSSGRef<QSSGRenderTextureCube> m_cubeCopy; ///< cube map buffer used during the blur passes
    QSSGRef<QSSGRenderTexture2D> m_depthRender; ///< shadow depth+stencil map used during rendering

    // RHI resources
    QRhiTexture *m_rhiDepthMap = nullptr; // shadow map (VSM)
    QRhiTexture *m_rhiDepthCopy = nullptr; // for blur pass (VSM)
    QRhiTexture *m_rhiDepthCube = nullptr; // shadow cube map (CUBE)
    QRhiTexture *m_rhiCubeCopy = nullptr; // for blur pass (CUBE)
    QRhiRenderBuffer *m_rhiDepthStencil = nullptr; // depth/stencil
    QVarLengthArray<QRhiTextureRenderTarget *, 6> m_rhiRenderTargets; // texture RT
    QRhiRenderPassDescriptor *m_rhiRenderPassDesc = nullptr; // texture RT renderpass descriptor
    QRhiTextureRenderTarget *m_rhiBlurRenderTarget0 = nullptr; // texture RT for blur X (targets depthCopy or cubeCopy)
    QRhiTextureRenderTarget *m_rhiBlurRenderTarget1 = nullptr; // texture RT for blur Y (targets depthMap or depthCube)
    QRhiRenderPassDescriptor *m_rhiBlurRenderPassDesc = nullptr; // blur needs its own because no depth/stencil

    QMatrix4x4 m_lightVP; ///< light view projection matrix
    QMatrix4x4 m_lightCubeView[6]; ///< light cubemap view matrices
    QMatrix4x4 m_lightView; ///< light view transform
};

class QSSGRenderShadowMap
{
    typedef QVector<QSSGShadowMapEntry> TShadowMapEntryList;

public:
    QAtomicInt ref;
    QSSGRef<QSSGRenderContextInterface> m_context;

    QSSGRenderShadowMap(const QSSGRef<QSSGRenderContextInterface> &inContext);
    ~QSSGRenderShadowMap();

    void addShadowMapEntry(qint32 index,
                           qint32 width,
                           qint32 height,
                           ShadowMapModes mode);

    QSSGShadowMapEntry *getShadowMapEntry(int index);

    qint32 getShadowMapEntryCount() { return m_shadowMapList.size(); }

    static QSSGRef<QSSGRenderShadowMap> create(const QSSGRef<QSSGRenderContextInterface> &inContext);

private:
    TShadowMapEntryList m_shadowMapList;
};
QT_END_NAMESPACE

#endif
