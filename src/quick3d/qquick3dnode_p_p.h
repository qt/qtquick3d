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

#ifndef QQUICK3DNODE_P_P_H
#define QQUICK3DNODE_P_P_H

#include <QtQuick3D/private/qtquick3dglobal_p.h>

#include "qquick3dobject_p_p.h"
#include "qquick3dnode_p.h"

#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>

QT_BEGIN_NAMESPACE

class QQuick3DNode;

class Q_QUICK3D_PRIVATE_EXPORT QQuick3DNodePrivate : public QQuick3DObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuick3DNode)

public:

    QQuick3DNodePrivate();
    ~QQuick3DNodePrivate();
    void init();

    QMatrix4x4 calculateLocalTransformRightHanded();
    void calculateGlobalVariables();
    void markGlobalTransformDirty();

    void emitChangesToGlobalTransform();
    bool isGlobalTransformRelatedSignal(const QMetaMethod &signal) const;

    static inline QQuick3DNodePrivate *get(QQuick3DNode *node) { return node->d_func(); }

    QVector3D m_rotation;
    QVector3D m_position;
    QVector3D m_scale{ 1.0f, 1.0f, 1.0f };
    QVector3D m_pivot;
    float m_opacity = 1.0f;
    qint32 m_boneid = -1;
    QQuick3DNode::RotationOrder m_rotationorder = QQuick3DNode::YXZ;
    QQuick3DNode::Orientation m_orientation = QQuick3DNode::LeftHanded;
    bool m_visible = true;
    QMatrix4x4 m_globalTransformRightHanded;
    bool m_globalTransformDirty = true;
    int m_globalTransformConnectionCount = 0;
};


QT_END_NAMESPACE

#endif // QQUICK3DNODE_P_P_H


