// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrcamera_p.h"
#include "qopenxrorigin_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3D/private/qquick3dutils_p.h>

#include <QtQuick3D/private/qquick3dnode_p_p.h>

QT_BEGIN_NAMESPACE

QOpenXREyeCamera::QOpenXREyeCamera(QQuick3DNode *parent)
    : QQuick3DCamera(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::CustomCamera)), parent)
{

}

float QOpenXREyeCamera::angleLeft() const
{
    return m_angleLeft;
}

float QOpenXREyeCamera::angleRight() const
{
    return m_angleRight;
}

float QOpenXREyeCamera::angleUp() const
{
    return m_angleUp;
}

float QOpenXREyeCamera::angleDown() const
{
    return m_angleDown;
}

float QOpenXREyeCamera::clipNear() const
{
    return m_clipNear;
}

float QOpenXREyeCamera::clipFar() const
{
    return m_clipFar;
}

void QOpenXREyeCamera::setAngleLeft(float angleLeft)
{
    if (qFuzzyCompare(m_angleLeft, angleLeft))
        return;

    m_angleLeft = angleLeft;
    emit angleLeftChanged(m_angleLeft);
    markProjectionDirty();
}

void QOpenXREyeCamera::setAngleRight(float angleRight)
{
    if (qFuzzyCompare(m_angleRight, angleRight))
        return;

    m_angleRight = angleRight;
    emit angleRightChanged(m_angleRight);
    markProjectionDirty();
}

void QOpenXREyeCamera::setAngleUp(float angleUp)
{
    if (qFuzzyCompare(m_angleUp, angleUp))
        return;

    m_angleUp = angleUp;
    emit angleUpChanged(m_angleUp);
    markProjectionDirty();
}

void QOpenXREyeCamera::setAngleDown(float angleDown)
{
    if (qFuzzyCompare(m_angleDown, angleDown))
        return;

    m_angleDown = angleDown;
    emit angleDownChanged(m_angleDown);
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

    const float tanLeft = qTan(m_angleLeft);
    const float tanRight = qTan(m_angleRight);

    const float tanDown = qTan(m_angleDown);
    const float tanUp = qTan(m_angleUp);

    const float tanAngleWidth = tanRight - tanLeft;
    const float tanAngleHeight = tanUp - tanDown;

    float *m = m_projection.data();


    m[0] = 2 / tanAngleWidth;
    m[4] = 0;
    m[8] = (tanRight + tanLeft) / tanAngleWidth;
    m[12] = 0;

    m[1] = 0;
    m[5] = 2 / tanAngleHeight;
    m[9] = (tanUp + tanDown) / tanAngleHeight;
    m[13] = 0;

    m[2] = 0;
    m[6] = 0;
    m[10] = -(m_clipFar + m_clipNear) / (m_clipFar - m_clipNear);
    m[14] = -2 *(m_clipFar * m_clipNear) / (m_clipFar - m_clipNear);

    m[3] = 0;
    m[7] = 0;
    m[11] = -1;
    m[15] = 0;
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
