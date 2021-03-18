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

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderdata_p.h>
#include <QtQuick3DRender/private/qssgrendershaderconstant_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderShadowMap::QSSGRenderShadowMap(const QSSGRef<QSSGRenderContextInterface> &inContext)
    : m_context(inContext)
{}

QSSGRenderShadowMap::~QSSGRenderShadowMap()
{
    m_shadowMapList.clear();
}

//static bool isDepthFormat(QSSGRenderTextureFormat format)
//{
//    switch (format.format) {
//    case QSSGRenderTextureFormat::Depth16:
//    case QSSGRenderTextureFormat::Depth24:
//    case QSSGRenderTextureFormat::Depth32:
//    case QSSGRenderTextureFormat::Depth24Stencil8:
//        return true;
//    default:
//        return false;
//    }
//}

void QSSGRenderShadowMap::addShadowMapEntry(qint32 index,
                                              qint32 width,
                                              qint32 height,
                                              QSSGRenderTextureFormat format,
                                              qint32 samples,
                                              ShadowMapModes mode,
                                              ShadowFilterValues filter)
{
    const QSSGRef<QSSGResourceManager> &theManager(m_context->resourceManager());
    QSSGShadowMapEntry *pEntry = nullptr;

    if (index < m_shadowMapList.size())
        pEntry = &m_shadowMapList[index];

    if (pEntry) {
        if ((nullptr != pEntry->m_depthMap) && (mode == ShadowMapModes::CUBE)) {
            theManager->release(pEntry->m_depthMap);
            theManager->release(pEntry->m_depthCopy);
            theManager->release(pEntry->m_depthRender);
            pEntry->m_depthCube = theManager->allocateTextureCube(width, height, format, samples);
            pEntry->m_cubeCopy = theManager->allocateTextureCube(width, height, format, samples);
            pEntry->m_depthRender = theManager->allocateTexture2D(width, height, QSSGRenderTextureFormat::Depth24Stencil8, samples);
            pEntry->m_depthMap = nullptr;
            pEntry->m_depthCopy = nullptr;
        } else if ((nullptr != pEntry->m_depthCube) && (mode != ShadowMapModes::CUBE)) {
            theManager->release(pEntry->m_depthCube);
            theManager->release(pEntry->m_cubeCopy);
            theManager->release(pEntry->m_depthRender);
            pEntry->m_depthMap = theManager->allocateTexture2D(width, height, format, samples);
            pEntry->m_depthCopy = theManager->allocateTexture2D(width, height, format, samples);
            pEntry->m_depthCube = nullptr;
            pEntry->m_cubeCopy = nullptr;
            pEntry->m_depthRender = theManager->allocateTexture2D(width, height, QSSGRenderTextureFormat::Depth24Stencil8, samples);
        } else if (nullptr != pEntry->m_depthMap) {
            QSSGTextureDetails theDetails(pEntry->m_depthMap->textureDetails());

            // If anything differs about the map we're looking for, let's recreate it.
            if (theDetails.format != format || theDetails.width != width || theDetails.height != height
                || theDetails.sampleCount != samples) {
                // release texture
                theManager->release(pEntry->m_depthMap);
                theManager->release(pEntry->m_depthCopy);
                theManager->release(pEntry->m_depthRender);
                pEntry->m_depthMap = theManager->allocateTexture2D(width, height, format, samples);
                pEntry->m_depthCopy = theManager->allocateTexture2D(width, height, format, samples);
                pEntry->m_depthCube = nullptr;
                pEntry->m_cubeCopy = nullptr;
                pEntry->m_depthRender = theManager->allocateTexture2D(width, height, QSSGRenderTextureFormat::Depth24Stencil8, samples);
            }
        } else {
            QSSGTextureDetails theDetails(pEntry->m_depthCube->textureDetails());

            // If anything differs about the map we're looking for, let's recreate it.
            if (theDetails.format != format || theDetails.width != width || theDetails.height != height
                || theDetails.sampleCount != samples) {
                // release texture
                theManager->release(pEntry->m_depthCube);
                theManager->release(pEntry->m_cubeCopy);
                theManager->release(pEntry->m_depthRender);
                pEntry->m_depthCube = theManager->allocateTextureCube(width, height, format, samples);
                pEntry->m_cubeCopy = theManager->allocateTextureCube(width, height, format, samples);
                pEntry->m_depthRender = theManager->allocateTexture2D(width, height, QSSGRenderTextureFormat::Depth24Stencil8, samples);
                pEntry->m_depthMap = nullptr;
                pEntry->m_depthCopy = nullptr;
            }
        }

        pEntry->m_shadowMapMode = mode;
        pEntry->m_shadowFilterFlags = filter;
    } else if (mode == ShadowMapModes::CUBE) {
        QSSGRef<QSSGRenderTextureCube> theDepthTex = theManager->allocateTextureCube(width, height, format, samples);
        QSSGRef<QSSGRenderTextureCube> theDepthCopy = theManager->allocateTextureCube(width, height, format, samples);
        QSSGRef<QSSGRenderTexture2D> theDepthTemp = theManager->allocateTexture2D(width,
                                                                                      height,
                                                                                      QSSGRenderTextureFormat::Depth24Stencil8,
                                                                                      samples);
        m_shadowMapList.push_back(QSSGShadowMapEntry(index, mode, filter, theDepthTex, theDepthCopy, theDepthTemp));

        pEntry = &m_shadowMapList.back();
    } else {
        QSSGRef<QSSGRenderTexture2D> theDepthMap = theManager->allocateTexture2D(width, height, format, samples);
        QSSGRef<QSSGRenderTexture2D> theDepthCopy = theManager->allocateTexture2D(width, height, format, samples);
        QSSGRef<QSSGRenderTexture2D> theDepthTemp = theManager->allocateTexture2D(width,
                                                                                      height,
                                                                                      QSSGRenderTextureFormat::Depth24Stencil8,
                                                                                      samples);
        m_shadowMapList.push_back(QSSGShadowMapEntry(index, mode, filter, theDepthMap, theDepthCopy, theDepthTemp));

        pEntry = &m_shadowMapList.back();
    }

    if (pEntry) {
        // setup some texture settings
        if (pEntry->m_depthMap) {
            pEntry->m_depthMap->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            pEntry->m_depthMap->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            pEntry->m_depthMap->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            pEntry->m_depthMap->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);

            pEntry->m_depthCopy->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            pEntry->m_depthCopy->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            pEntry->m_depthCopy->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            pEntry->m_depthCopy->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);

            pEntry->m_depthRender->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            pEntry->m_depthRender->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            pEntry->m_depthRender->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            pEntry->m_depthRender->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);
        } else {
            pEntry->m_depthCube->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            pEntry->m_depthCube->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            pEntry->m_depthCube->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            pEntry->m_depthCube->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);

            pEntry->m_cubeCopy->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            pEntry->m_cubeCopy->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            pEntry->m_cubeCopy->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            pEntry->m_cubeCopy->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);

            pEntry->m_depthRender->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            pEntry->m_depthRender->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            pEntry->m_depthRender->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            pEntry->m_depthRender->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);
        }

        pEntry->m_lightIndex = index;
    }
}

QSSGShadowMapEntry *QSSGRenderShadowMap::getShadowMapEntry(int index)
{
    if (index < 0) {
        Q_UNREACHABLE();
        return nullptr;
    }
    QSSGShadowMapEntry *pEntry = nullptr;

    for (int i = 0; i < m_shadowMapList.size(); i++) {
        pEntry = &m_shadowMapList[i];
        if (pEntry->m_lightIndex == quint32(index))
            return pEntry;
    }

    return nullptr;
}

QSSGRef<QSSGRenderShadowMap> QSSGRenderShadowMap::create(const QSSGRef<QSSGRenderContextInterface> &inContext)
{
    return QSSGRef<QSSGRenderShadowMap>(new QSSGRenderShadowMap(inContext));
}

QT_END_NAMESPACE
