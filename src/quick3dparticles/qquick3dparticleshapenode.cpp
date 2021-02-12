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

#include "qquick3dparticleshapenode_p.h"

QT_BEGIN_NAMESPACE

QQuick3DParticleShapeNode::QQuick3DParticleShapeNode(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{
    // TODO: If parent isn't a node, should we reach for parents parent etc?
}

QQuick3DParticleShape *QQuick3DParticleShapeNode::shape() const
{
    return m_shape;
}

void QQuick3DParticleShapeNode::setShape(QQuick3DParticleShape *shape)
{
    if (m_shape == shape)
        return;

    m_shape = shape;
    Q_EMIT shapeChanged();
}

QT_END_NAMESPACE
