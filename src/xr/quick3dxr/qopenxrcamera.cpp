// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrcamera_p.h"
#include "qopenxrorigin_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3D/private/qquick3dutils_p.h>
#include <QtQuick3D/private/qquick3dnode_p_p.h>

#include <cmath>

QT_BEGIN_NAMESPACE

QOpenXREyeCamera::QOpenXREyeCamera(QQuick3DNode *parent)
    : QQuick3DCamera(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::CustomCamera)), parent)
{

}

float QOpenXREyeCamera::leftTangent() const
{
    return m_leftTangent;
}

float QOpenXREyeCamera::rightTangent() const
{
    return m_rightTangent;
}

float QOpenXREyeCamera::upTangent() const
{
    return m_upTangent;
}

float QOpenXREyeCamera::downTangent() const
{
    return m_downTangent;
}

float QOpenXREyeCamera::clipNear() const
{
    return m_clipNear;
}

float QOpenXREyeCamera::clipFar() const
{
    return m_clipFar;
}

void QOpenXREyeCamera::setLeftTangent(float leftTangent)
{
    if (qFuzzyCompare(m_leftTangent, leftTangent))
        return;

    m_leftTangent = leftTangent;
    emit leftTangentChanged(m_leftTangent);
    markProjectionDirty();
}

void QOpenXREyeCamera::setRightTangent(float rightTangent)
{
    if (qFuzzyCompare(m_rightTangent, rightTangent))
        return;

    m_rightTangent = rightTangent;
    emit rightTangentChanged(m_rightTangent);
    markProjectionDirty();
}

void QOpenXREyeCamera::setUpTangent(float upTangent)
{
    if (qFuzzyCompare(m_upTangent, upTangent))
        return;

    m_upTangent = upTangent;
    emit upTangentChanged(m_upTangent);
    markProjectionDirty();
}

void QOpenXREyeCamera::setDownTangent(float downTangent)
{
    if (qFuzzyCompare(m_downTangent, downTangent))
        return;

    m_downTangent = downTangent;
    emit downTangentChanged(m_downTangent);
    markProjectionDirty();
}

void QOpenXREyeCamera::setClipNear(float clipNear)
{
    if (qFuzzyCompare(m_clipNear, clipNear))
        return;

    m_clipNear = clipNear;
    emit clipNearChanged(m_clipNear);
    markProjectionDirty();
}

void QOpenXREyeCamera::setClipFar(float clipFar)
{
    if (qFuzzyCompare(m_clipFar, clipFar))
        return;

    m_clipFar = clipFar;
    emit clipFarChanged(m_clipFar);
    markProjectionDirty();
}

QSSGRenderGraphObject *QOpenXREyeCamera::updateSpatialNode(QSSGRenderGraphObject *node)
{
    QSSGRenderCamera *camera = static_cast<QSSGRenderCamera *>(QQuick3DCamera::updateSpatialNode(node));
    if (camera) {
        maybeUpdateProjection();
        bool changed = false;
        changed |= qUpdateIfNeeded(camera->projection, m_projection);
        // Still need to report near and far (used for shadows)
        changed |= qUpdateIfNeeded(camera->clipNear, m_clipNear);
        changed |= qUpdateIfNeeded(camera->clipFar, m_clipFar);
        if (changed)
            camera->markDirty(QSSGRenderCamera::DirtyFlag::CameraDirty);
    }
    return camera;
}

void QOpenXREyeCamera::markProjectionDirty()
{
    if (!m_projectionDirty) {
        m_projectionDirty = true;
        update();
    }
}

void QOpenXREyeCamera::maybeUpdateProjection()
{
    if (!m_projectionDirty)
        return;

    const float right = m_rightTangent * m_clipNear;
    const float top = m_upTangent * m_clipNear;
#if defined(Q_OS_VISIONOS)
    // cp_view_get_tangents always returns positive values
    // so we need to negate the left and down tangents
    const float left = -m_leftTangent * m_clipNear;
    const float bottom = -m_downTangent * m_clipNear;
#else
    const float left = m_leftTangent * m_clipNear;
    const float bottom = m_downTangent * m_clipNear;
#endif

    float *m = m_projection.data();

    m[0] = 2 * m_clipNear / (right - left);
    m[4] = 0;
    m[8] = (right + left) / (right - left);
    m[12] = 0;

    m[1] = 0;
    m[5] = 2 * m_clipNear / (top - bottom);
    m[9] = (top + bottom) / (top - bottom);
    m[13] = 0;

    m[2] = 0;
    m[6] = 0;
    m[10] = m_clipFar / (m_clipNear - m_clipFar);
    m[14] = m_clipFar * m_clipNear / (m_clipNear - m_clipFar);


    m[3] = 0;
    m[7] = 0;
    m[11] = -1;
    m[15] = 0;

    const bool isReverseZ = false; // placeholder
    if (isReverseZ) {
        if (std::isinf(m_clipFar)) {
            m[10] = 0;
            m[14] = m_clipNear;
        } else {
            m[10] = -m[10] - 1;
            m[14] = -m[14];
        }
    } else if (std::isinf(m_clipFar)) {
        m[10] = -1;
        m[14] = -m_clipNear;
    }
}

QOpenXRCamera::QOpenXRCamera(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{
}

QOpenXRCamera::~QOpenXRCamera()
{

}

float QOpenXRCamera::clipNear() const
{
    return m_clipNear;
}

float QOpenXRCamera::clipFar() const
{
    return m_clipFar;
}


void QOpenXRCamera::setClipNear(float clipNear)
{
    if (qFuzzyCompare(m_clipNear, clipNear))
        return;
    m_clipNear = clipNear;
    emit clipNearChanged(m_clipNear);
}

void QOpenXRCamera::setClipFar(float clipFar)
{
    if (qFuzzyCompare(m_clipFar, clipFar))
        return;
    m_clipFar = clipFar;
    emit clipFarChanged(m_clipFar);
}

QT_END_NAMESPACE
