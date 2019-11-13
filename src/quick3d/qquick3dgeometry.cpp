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

#include "qquick3dgeometry_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dobject_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

/*!
    \qmltype Geometry
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief An Abstract base type for custom geometry.

    \sa Model
*/
/*!
    \qmlproperty string Geometry::name
    Unique name identifying the geometry.
*/

QQuick3DGeometry::QQuick3DGeometry()
{

}

QQuick3DGeometry::~QQuick3DGeometry()
{

}

QQuick3DObject::Type QQuick3DGeometry::type() const
{
    return QQuick3DObject::Geometry;
}

QString QQuick3DGeometry::name() const
{
    return m_name;
}

void QQuick3DGeometry::setName(const QString &name)
{
    if (name != m_name) {
        m_nameChanged = true;
        m_name = name;
        Q_EMIT nameChanged();
        update();
    }
}

QSSGRenderGraphObject *QQuick3DGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderGeometry();

    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);
    if (m_nameChanged) {
        geometry->setPath(m_name);
        m_nameChanged = false;
    }
    return node;
}

