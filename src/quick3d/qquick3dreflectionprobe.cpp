/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qquick3dreflectionprobe_p.h"
#include "qquick3dnode_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderreflectionprobe_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE
QQuick3DReflectionProbe::QQuick3DReflectionProbe(QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::ReflectionProbe)), parent)
{
}

QQuick3DReflectionProbe::ReflectionQuality QQuick3DReflectionProbe::quality() const
{
    return m_quality;
}

QColor QQuick3DReflectionProbe::clearColor() const
{
    return m_clearColor;
}

QQuick3DReflectionProbe::ReflectionRefreshMode QQuick3DReflectionProbe::refreshMode() const
{
    return m_refreshMode;
}

QQuick3DReflectionProbe::ReflectionTimeSlicing QQuick3DReflectionProbe::timeSlicing() const
{
    return m_timeSlicing;
}

bool QQuick3DReflectionProbe::parallaxCorrection() const
{
    return m_parallaxCorrection;
}

QVector3D QQuick3DReflectionProbe::boxSize() const
{
    return m_boxSize;
}

void QQuick3DReflectionProbe::setQuality(QQuick3DReflectionProbe::ReflectionQuality reflectionQuality)
{
    if (m_quality == reflectionQuality)
        return;

    m_quality = reflectionQuality;
    m_dirtyFlags.setFlag(DirtyFlag::QualityDirty);
    emit qualityChanged();
    update();
}

void QQuick3DReflectionProbe::setClearColor(const QColor &clearColor)
{
    if (m_clearColor == clearColor)
        return;
    m_clearColor = clearColor;
    m_dirtyFlags.setFlag(DirtyFlag::ClearColorDirty);
    emit clearColorChanged();
    update();
}

void QQuick3DReflectionProbe::setRefreshMode(ReflectionRefreshMode newRefreshMode)
{
    if (m_refreshMode == newRefreshMode)
        return;
    m_refreshMode = newRefreshMode;
    m_dirtyFlags.setFlag(DirtyFlag::RefreshModeDirty);
    emit refreshModeChanged();
    update();
}

void QQuick3DReflectionProbe::setTimeSlicing(ReflectionTimeSlicing newTimeSlicing)
{
    if (m_timeSlicing == newTimeSlicing)
        return;
    m_timeSlicing = newTimeSlicing;
    m_dirtyFlags.setFlag(DirtyFlag::TimeSlicingDirty);
    emit timeSlicingChanged();
    update();
}

void QQuick3DReflectionProbe::setParallaxCorrection(bool parallaxCorrection)
{
    if (m_parallaxCorrection == parallaxCorrection)
        return;
    m_parallaxCorrection = parallaxCorrection;
    m_dirtyFlags.setFlag(DirtyFlag::ParallaxCorrectionDirty);
    emit parallaxCorrectionChanged();
    update();
}

void QQuick3DReflectionProbe::setBoxSize(const QVector3D &boxSize)
{
    if (m_boxSize == boxSize)
        return;
    m_boxSize = boxSize;
    m_dirtyFlags.setFlag(DirtyFlag::BoxDirty);
    emit boxSizeChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DReflectionProbe::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderReflectionProbe();
    }

    QQuick3DNode::updateSpatialNode(node);

    QSSGRenderReflectionProbe *probe = static_cast<QSSGRenderReflectionProbe *>(node);

    if (m_dirtyFlags.testFlag(DirtyFlag::QualityDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::QualityDirty, false);
        probe->reflectionMapRes = mapToReflectionResolution(m_quality);
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::ClearColorDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::ClearColorDirty, false);
        probe->clearColor = m_clearColor;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::RefreshModeDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::RefreshModeDirty, false);
        switch (m_refreshMode) {
        case ReflectionRefreshMode::FirstFrame:
            probe->refreshMode = QSSGRenderReflectionProbe::ReflectionRefreshMode::FirstFrame;
            break;
        case ReflectionRefreshMode::EveryFrame:
            probe->refreshMode = QSSGRenderReflectionProbe::ReflectionRefreshMode::EveryFrame;
            break;
        }
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::TimeSlicingDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::TimeSlicingDirty, false);
        switch (m_timeSlicing) {
        case ReflectionTimeSlicing::None:
            probe->timeSlicing = QSSGRenderReflectionProbe::ReflectionTimeSlicing::None;
            break;
        case ReflectionTimeSlicing::AllFacesAtOnce:
            probe->timeSlicing = QSSGRenderReflectionProbe::ReflectionTimeSlicing::AllFacesAtOnce;
            break;
        case ReflectionTimeSlicing::IndividualFaces:
            probe->timeSlicing = QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces;
            break;
        }
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::ParallaxCorrectionDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::ParallaxCorrectionDirty, false);
        probe->parallaxCorrection = m_parallaxCorrection;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::BoxDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::BoxDirty, false);
        probe->boxSize = m_boxSize;
    }

    return node;
}

void QQuick3DReflectionProbe::markAllDirty()
{
    m_dirtyFlags = DirtyFlags(DirtyFlag::QualityDirty)
            | DirtyFlags(DirtyFlag::ClearColorDirty)
            | DirtyFlags(DirtyFlag::RefreshModeDirty)
            | DirtyFlags(DirtyFlag::ParallaxCorrectionDirty)
            | DirtyFlags(DirtyFlag::BoxDirty)
            | DirtyFlags(DirtyFlag::TimeSlicingDirty);
    QQuick3DNode::markAllDirty();
}

quint32 QQuick3DReflectionProbe::mapToReflectionResolution(ReflectionQuality quality)
{
    switch (quality) {
    case ReflectionQuality::Medium:
        return 9;
    case ReflectionQuality::High:
        return 10;
    case ReflectionQuality::VeryHigh:
        return 11;
    default:
        break;
    }
    return 8;
}

QT_END_NAMESPACE
