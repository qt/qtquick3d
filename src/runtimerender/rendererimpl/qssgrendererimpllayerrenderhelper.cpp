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

#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderhelper_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

QT_BEGIN_NAMESPACE

#if 0
// left/top
inline constexpr float getMinValue(float start, float width, float value, QSSGRenderLayer::UnitType units) Q_DECL_NOTHROW
{
    return (units == QSSGRenderLayer::UnitType::Pixels) ? (start + value) : (start + (value * width / 100.0f));
}

// width/height
inline constexpr float getValueLen(float width, float value, QSSGRenderLayer::UnitType units) Q_DECL_NOTHROW
{
    return (units == QSSGRenderLayer::UnitType::Pixels) ? value : (width * value / 100.0f);
}

// right/bottom
inline constexpr float getMaxValue(float start, float width, float value, QSSGRenderLayer::UnitType units) Q_DECL_NOTHROW
{
    return (units == QSSGRenderLayer::UnitType::Pixels) ? (start + width - value) : (start + width - (value * width / 100.0f));
}
#endif

QSSGLayerRenderHelper::QSSGLayerRenderHelper(const QRectF &inPresentationViewport,
                                                 const QRectF &inPresentationScissor,
                                                 QSSGRenderLayer &inLayer,
                                                 bool inOffscreen,
                                                 ScaleModes inScaleMode,
                                                 QVector2D inScaleFactor)
    : m_presentationViewport(inPresentationViewport)
    , m_presentationScissor(inPresentationScissor)
    , m_layer(&inLayer)
    , m_offscreen(inOffscreen)
    , m_scaleMode(inScaleMode)
    , m_scaleFactor(inScaleFactor)
{
    // The following code is used to get the correct viewport for the layer compositor.
    // We dont actually use this code anymore, so for now it is just disabled until
    // we figure out whether we want to use this code.
//    {
//        float left = m_layer->m_left;
//        float right = m_layer->m_right;
//        float width = m_layer->m_width;

//        if (m_scaleMode == ScaleModes::FitSelected) {
//            if (m_layer->leftUnits == QSSGRenderLayer::UnitType::Pixels)
//                left *= m_scaleFactor.x();

//            if (m_layer->rightUnits == QSSGRenderLayer::UnitType::Pixels)
//                right *= m_scaleFactor.x();

//            if (m_layer->widthUnits == QSSGRenderLayer::UnitType::Pixels)
//                width *= m_scaleFactor.x();
//        }

//        float horzMin = getMinValue(inPresentationViewport.x(), inPresentationViewport.width(), left, m_layer->leftUnits);
//        float horzWidth = getValueLen(inPresentationViewport.width(), width, m_layer->widthUnits);
//        float horzMax = getMaxValue(inPresentationViewport.x(), inPresentationViewport.width(), right, m_layer->rightUnits);

//        switch (inLayer.horizontalFieldValues) {
//        case QSSGRenderLayer::HorizontalField::LeftWidth:
//            m_viewport.setX(horzMin);
//            m_viewport.setWidth(horzWidth);
//            break;
//        case QSSGRenderLayer::HorizontalField::LeftRight:
//            m_viewport.setX(horzMin);
//            m_viewport.setWidth(horzMax - horzMin);
//            break;
//        case QSSGRenderLayer::HorizontalField::WidthRight:
//            m_viewport.setWidth(horzWidth);
//            m_viewport.setX(horzMax - horzWidth);
//            break;
//        }
//    }
//    {
//        float top = m_layer->m_top;
//        float bottom = m_layer->m_bottom;
//        float height = m_layer->m_height;

//        if (m_scaleMode == ScaleModes::FitSelected) {

//            if (m_layer->topUnits == QSSGRenderLayer::UnitType::Pixels)
//                top *= m_scaleFactor.y();

//            if (m_layer->bottomUnits == QSSGRenderLayer::UnitType::Pixels)
//                bottom *= m_scaleFactor.y();

//            if (m_layer->heightUnits == QSSGRenderLayer::UnitType::Pixels)
//                height *= m_scaleFactor.y();
//        }

//        float vertMin = getMinValue(inPresentationViewport.y(), inPresentationViewport.height(), bottom, m_layer->bottomUnits);
//        float vertWidth = getValueLen(inPresentationViewport.height(), height, m_layer->heightUnits);
//        float vertMax = getMaxValue(inPresentationViewport.y(), inPresentationViewport.height(), top, m_layer->topUnits);

//        switch (inLayer.verticalFieldValues) {
//        case QSSGRenderLayer::VerticalField::HeightBottom:
//            m_viewport.setY(vertMin);
//            m_viewport.setHeight(vertWidth);
//            break;
//        case QSSGRenderLayer::VerticalField::TopBottom:
//            m_viewport.setY(vertMin);
//            m_viewport.setHeight(vertMax - vertMin);
//            break;
//        case QSSGRenderLayer::VerticalField::TopHeight:
//            m_viewport.setHeight(vertWidth);
//            m_viewport.setY(vertMax - vertWidth);
//            break;
//        }
//    }
    m_viewport = m_presentationViewport;

    m_viewport.setWidth(qMax<qreal>(1.0f, m_viewport.width()));
    m_viewport.setHeight(qMax<qreal>(1.0f, m_viewport.height()));
    // Now force the viewport to be a multiple of four in width and height.  This is because
    // when rendering to a texture we have to respect this and not forcing it causes scaling issues
    // that are noticeable especially in situations where customers are using text and such.
    float originalWidth = m_viewport.width();
    float originalHeight = m_viewport.height();

    m_viewport.setWidth((float)QSSGRendererUtil::nextMultipleOf4((quint32)m_viewport.width()));
    m_viewport.setHeight((float)QSSGRendererUtil::nextMultipleOf4((quint32)m_viewport.height()));

    // Now fudge the offsets to account for this slight difference
    m_viewport.setX(m_viewport.x() + (originalWidth - m_viewport.width()) / 2.0f);
    m_viewport.setY(m_viewport.y() + (originalHeight - m_viewport.height()) / 2.0f);

    m_scissor = m_viewport;
    m_scissor &= inPresentationScissor; // ensureInBounds/intersected
    Q_ASSERT(m_scissor.width() >= 0.0f);
    Q_ASSERT(m_scissor.height() >= 0.0f);
}

// This is the viewport the camera will use to setup the projection.
QRectF QSSGLayerRenderHelper::layerRenderViewport() const
{
    if (m_offscreen)
        return QRectF(0, 0, m_viewport.width(), (float)m_viewport.height());
    else
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
    m_camera = &inCamera;
    QRectF rect = layerRenderViewport();
    if (m_scaleMode == ScaleModes::FitSelected) {
        rect.setWidth((float)(QSSGRendererUtil::nextMultipleOf4((quint32)(rect.width() / m_scaleFactor.x()))));
        rect.setHeight((float)(QSSGRendererUtil::nextMultipleOf4((quint32)(rect.height() / m_scaleFactor.y()))));
    }
    return m_camera->calculateGlobalVariables(rect);
}

QSSGOption<QVector2D> QSSGLayerRenderHelper::layerMouseCoords(const QVector2D &inMouseCoords,
                                                                     const QVector2D &inWindowDimensions,
                                                                     bool inForceIntersect) const
{
    // First invert the y so we are dealing with numbers in a normal coordinate space.
    // Second, move into our layer's coordinate space
    QVector2D correctCoords(inMouseCoords.x(), inWindowDimensions.y() - inMouseCoords.y());
    QVector2D theLocalMouse = toRectRelative(m_viewport, correctCoords);

    float theRenderRectWidth = m_viewport.width();
    float theRenderRectHeight = m_viewport.height();
    // Crop the mouse to the rect.  Apply no further translations.
    if (inForceIntersect == false
        && (theLocalMouse.x() < 0.0f || theLocalMouse.x() >= theRenderRectWidth || theLocalMouse.y() < 0.0f
            || theLocalMouse.y() >= theRenderRectHeight)) {
        return QSSGEmpty();
    }
    return theLocalMouse;
}

QSSGOption<QSSGRenderRay> QSSGLayerRenderHelper::pickRay(const QVector2D &inMouseCoords,
                                                                  const QVector2D &inWindowDimensions,
                                                                  bool inForceIntersect) const
{
    if (m_camera == nullptr)
        return QSSGEmpty();
    QSSGOption<QVector2D> theCoords(layerMouseCoords(inMouseCoords, inWindowDimensions, inForceIntersect));
    if (theCoords.hasValue()) {
        // The cameras projection is different if we are onscreen vs. offscreen.
        // When offscreen, we need to move the mouse coordinates into a local space
        // to the layer.
        return m_camera->unproject(*theCoords, m_viewport);
    }
    return QSSGEmpty();
}

bool QSSGLayerRenderHelper::isLayerVisible() const
{
    return m_scissor.height() >= 2.0f && m_scissor.width() >= 2.0f;
}

QT_END_NAMESPACE
