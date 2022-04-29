/****************************************************************************
**
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
                                       const QVector3D &scenePosition,
                                       const QVector3D &position,
                                       const QVector3D &normal)
    : m_objectHit(hitObject)
    , m_distance(distanceFromCamera)
    , m_uvPosition(uvPosition)
    , m_scenePosition(scenePosition)
    , m_position(position)
    , m_normal(normal)
{
}

/*!
    \qmlproperty Model PickResult::objectHit
    \readonly

    This property holds the model object hit by the pick.
*/
QQuick3DModel *QQuick3DPickResult::objectHit() const
{
    return m_objectHit;
}

/*!
    \qmlproperty float PickResult::distance
    \readonly

    This property holds the distance between the pick origin and the hit position
    i.e. the length of the ray. In the case of using viewport coordinates for
    picking the pick origin will be the active camera's position.
*/
float QQuick3DPickResult::distance() const
{
    return m_distance;
}

/*!
    \qmlproperty vector2d PickResult::uvPosition
    \readonly

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
    \readonly

    This property holds the scene position of the hit.
*/
QVector3D QQuick3DPickResult::scenePosition() const
{
    return m_scenePosition;
}

/*!
    \qmlproperty vector3d PickResult::position
    \readonly

    This property holds the scene position of the hit in local coordinate
    space.
*/
QVector3D QQuick3DPickResult::position() const
{
    return m_position;
}

/*!
    \qmlproperty vector3d PickResult::normal
    \readonly

    This property holds the normal of the face that was hit in local coordinate
    space.
*/
QVector3D QQuick3DPickResult::normal() const
{
    return m_normal;
}


/*!
    \qmlproperty vector3d PickResult::sceneNormal
    \readonly

    This property holds the normal of the face that was hit in scene coordinate
    space.
*/
QVector3D QQuick3DPickResult::sceneNormal() const
{
    if (!m_objectHit)
        return QVector3D();

    return m_objectHit->mapDirectionToScene(m_normal);
}

QT_END_NAMESPACE
