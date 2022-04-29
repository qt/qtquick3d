/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderhelper_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

QT_BEGIN_NAMESPACE

QSSGLayerRenderHelper::QSSGLayerRenderHelper(const QRectF &inViewport,
                                                 const QRectF &inScissor,
                                                 QSSGRenderLayer &inLayer)
    : m_layer(&inLayer)
{

    m_viewport = inViewport;

    m_scissor = m_viewport;
    m_scissor &= inScissor; // ensureInBounds/intersected
    Q_ASSERT(m_scissor.width() >= 0.0f);
    Q_ASSERT(m_scissor.height() >= 0.0f);
}

// This is the viewport the camera will use to setup the projection.
QRectF QSSGLayerRenderHelper::layerRenderViewport() const
{
    return m_viewport;
}

QSize QSSGLayerRenderHelper::textureDimensions() const
{
    quint32 width = (quint32)m_viewport.width();
    quint32 height = (quint32)m_viewport.height();
    return QSize(QSSGRendererUtil::nextMultipleOf4(width), QSSGRendererUtil::nextMultipleOf4(height));
}

QSSGCameraGlobalCalculationResult QSSGLayerRenderHelper::setupCameraForRender(QSSGRenderCamera &inCamera)
{
    return inCamera.calculateGlobalVariables(layerRenderViewport());
}

QSSGOption<QVector2D> QSSGLayerRenderHelper::layerMouseCoords(const QRectF &viewport,
                                                              const QVector2D &inMouseCoords,
                                                              const QVector2D &inWindowDimensions,
                                                              bool inForceIntersect)
{
    // First invert the y so we are dealing with numbers in a normal coordinate space.
    // Second, move into our layer's coordinate space
    QVector2D correctCoords(inMouseCoords.x(), inWindowDimensions.y() - inMouseCoords.y());
    QVector2D theLocalMouse = toRectRelative(viewport, correctCoords);

    const float theRenderRectWidth = float(viewport.width());
    const float theRenderRectHeight = float(viewport.height());
    // Crop the mouse to the rect.  Apply no further translations.
    if (!inForceIntersect
        && (theLocalMouse.x() < 0.0f || theLocalMouse.x() >= theRenderRectWidth || theLocalMouse.y() < 0.0f
            || theLocalMouse.y() >= theRenderRectHeight)) {
        return QSSGEmpty();
    }
    return theLocalMouse;
}

QSSGOption<QSSGRenderRay> QSSGLayerRenderHelper::pickRay(const QSSGRenderCamera &camera,
                                                         const QRectF &viewport,
                                                         const QVector2D &inMouseCoords,
                                                         const QVector2D &inWindowDimensions,
                                                         bool inForceIntersect)
{
    QSSGOption<QVector2D> theCoords(layerMouseCoords(viewport, inMouseCoords, inWindowDimensions, inForceIntersect));
    if (theCoords.hasValue()) {
        // The cameras projection is different if we are onscreen vs. offscreen.
        // When offscreen, we need to move the mouse coordinates into a local space
        // to the layer.
        return camera.unproject(*theCoords, viewport);
    }
    return QSSGEmpty();
}

bool QSSGLayerRenderHelper::isLayerVisible() const
{
    return m_scissor.height() >= 2.0f && m_scissor.width() >= 2.0f;
}

QT_END_NAMESPACE
