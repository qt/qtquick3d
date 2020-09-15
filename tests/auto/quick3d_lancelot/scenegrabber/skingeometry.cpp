/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "skingeometry.h"

#include <qmath.h>

struct Vertex {
    QVector3D position;
    qint32 joints[4];
    float weights[4];

    float pad;
};

static const int s_vertexSize = sizeof(Vertex);
Q_STATIC_ASSERT(s_vertexSize == 48);

SkinGeometry::SkinGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::ComponentType::U32Type);
    addAttribute(QQuick3DGeometry::Attribute::JointSemantic, offsetof(Vertex, joints[0]),
                 QQuick3DGeometry::Attribute::ComponentType::I32Type);
    addAttribute(QQuick3DGeometry::Attribute::WeightSemantic, offsetof(Vertex, weights[0]),
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);
}

QList<QVector3D> SkinGeometry::positions() const
{
    return m_positions;
}

QList<qint32> SkinGeometry::joints() const
{
    return m_joints;
}

QList<float> SkinGeometry::weights() const
{
    return m_weights;
}

QList<quint32> SkinGeometry::indexes() const
{
    return m_indexes;
}

void SkinGeometry::setPositions(const QList<QVector3D> &positions)
{
    if (positions == m_positions)
        return;
    m_positions = positions;
    emit positionsChanged();
    m_vertexDirty = true;
}

void SkinGeometry::setJoints(const QList<qint32> &joints)
{
    if (joints == m_joints)
        return;
    m_joints = joints;
    emit jointsChanged();
    m_vertexDirty = true;
}

void SkinGeometry::setWeights(const QList<float> &weights)
{
    if (weights == m_weights)
        return;
    m_weights = weights;
    emit weightsChanged();
    m_vertexDirty = true;
}

void SkinGeometry::setIndexes(const QList<quint32> &indexes)
{
    if (indexes == m_indexes)
        return;
    m_indexes = indexes;
    emit indexesChanged();
    m_indexDirty = true;
}

QSSGRenderGraphObject *SkinGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_vertexDirty) {
        m_vertexDirty = false;

        const int numVertexes = m_positions.size();
        m_vertexBuffer.resize(numVertexes * s_vertexSize);
        Vertex *vert = reinterpret_cast<Vertex *>(m_vertexBuffer.data());

        for (int i = 0; i < numVertexes; ++i) {
            Vertex &v = vert[i];
            v.position = m_positions[i];
            if (m_joints.size() >= 4 * (i + 1))
                memcpy(v.joints, m_joints.constData() + 4 * i, 4 * sizeof(qint32));
            else
                v.joints[0] = v.joints[1] = v.joints[2] = v.joints[3] = 0;
            if (m_weights.size() >= 4 * (i + 1))
                memcpy(v.weights, m_weights.constData() + 4 * i, 4 * sizeof(float));
            else
                v.weights[0] = v.weights[1] = v.weights[2] = v.weights[3] = 0.0f;
        }

        setStride(s_vertexSize);
        setVertexData(m_vertexBuffer);
        setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    }

    if (m_indexDirty) {
        m_indexDirty = false;
        m_indexBuffer = QByteArray(reinterpret_cast<char *>(m_indexes.data()), m_indexes.size() * sizeof(quint32));
        setIndexData(m_indexBuffer);
    }

    node = QQuick3DGeometry::updateSpatialNode(node);
    return node;
}

