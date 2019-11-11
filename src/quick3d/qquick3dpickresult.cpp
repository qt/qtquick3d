/****************************************************************************
**
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

#include "qquick3dpickresult_p.h"
#include "qquick3dmodel_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype PickResult
    \inqmlmodule QtQuick3D
    \brief Contains the results of a pick.

    Created as a return object to View3D::pick.
*/

QQuick3DPickResult::QQuick3DPickResult()
    : m_objectHit(nullptr)
    , m_distance(0.0f)
{

}

QQuick3DPickResult::QQuick3DPickResult(QQuick3DModel *hitObject,
                                       float distanceFromCamera,
                                       const QVector2D &uvPosition,
                                       const QVector3D &scenePosition)
    : m_objectHit(hitObject)
    , m_distance(distanceFromCamera)
    , m_uvPosition(uvPosition)
    , m_scenePosition(scenePosition)
{
}

QQuick3DPickResult::QQuick3DPickResult(const QQuick3DPickResult &obj)
    : m_objectHit(obj.m_objectHit)
    , m_distance(obj.m_distance)
    , m_uvPosition(obj.m_uvPosition)
    , m_scenePosition(obj.m_scenePosition)
{
}

QQuick3DPickResult::~QQuick3DPickResult()
{
}

/*!
    \qmlproperty Model PickResult::objectHit

    This property holds the model object hit by the pick.
*/
QQuick3DModel *QQuick3DPickResult::objectHit() const
{
    return m_objectHit;
}

/*!
    \qmlproperty float PickResult::distance

    This property holds the distance between the camera and the hit position
    i.e. the length of the ray.
*/
float QQuick3DPickResult::distance() const
{
    return m_distance;
}

/*!
    \qmlproperty vector2d PickResult::uvPosition

    This property holds the UV position of the hit. The UV position is calculated as
    the normalized local x and y coordinates of the hit point relative to the bounding volume.
    Useful for further picking against an offscreen-rendered object.
*/
QVector2D QQuick3DPickResult::uvPosition() const
{
    return m_uvPosition;
}

/*!
    \qmlproperty vector3d PickResult::scenePosition

    This property holds the scene position of the hit.
*/
QVector3D QQuick3DPickResult::scenePosition() const
{
    return m_scenePosition;
}

QT_END_NAMESPACE
