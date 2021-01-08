/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "morphgeometry.h"

#include <qmath.h>

struct Vertex {
    QVector3D position;
    QVector3D normal;

    QVector3D targetPosition;
    QVector3D targetNormal;
};

Q_STATIC_ASSERT((sizeof(Vertex)/16)*16 == sizeof(Vertex)); // must be  4-float aligned

MorphGeometry::MorphGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float),
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);

    addAttribute(QQuick3DGeometry::Attribute::TargetPositionSemantic, 6 * sizeof(float),
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TargetNormalSemantic, 9 * sizeof(float),
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);

    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::ComponentType::U32Type);
}

QSSGRenderGraphObject *MorphGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_geometryDirty) {
        m_geometryDirty = false;
        calculateGeometry();

        const int numVertexes = m_positions.size();
        m_vertexBuffer.resize(numVertexes * sizeof(Vertex));
        Vertex *vert = reinterpret_cast<Vertex *>(m_vertexBuffer.data());

        for (int i = 0; i < numVertexes; ++i) {
            Vertex &v = vert[i];
            v.position = m_positions[i];
            v.normal = m_normals[i];
            v.targetPosition = m_targetPositions[i];
            v.targetNormal = m_targetNormals[i];
        }

        setStride(sizeof(Vertex));
        setVertexData(m_vertexBuffer);
        setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

        m_indexBuffer = QByteArray(reinterpret_cast<char *>(m_indexes.data()), m_indexes.size() * sizeof(quint32));
        setIndexData(m_indexBuffer);
    }

    node = QQuick3DGeometry::updateSpatialNode(node);
    return node;
}

void MorphGeometry::calculateGeometry()
{
    float dw = 10.0; // width/2
    float dh = 10.0;
    int iw = 50; // grid size
    int ih = 50;
    float wf = dw * 2 / iw; //factor grid coord => position
    float hf = dh * 2 / ih;

    m_positions.clear();
    m_indexes.clear();
    m_targetPositions.clear();
    m_targetNormals.clear();

    for (int iy = 0; iy < ih; ++iy) {
        for (int ix = 0; ix < iw; ++ix) {
            float x = ix * wf - dw;
            float z = iy * hf - dh;

            float y = 2 * qCos(x) + 3.0;
            QVector3D normal{2 * qSin(x), 2, 0};

            float tx = x * 1.2;
            float tz = z * 1.2;

            constexpr float R = 16;
            QVector3D targetPosition;
            QVector3D targetNormal;

            if (tx*tx + tz*tz < R*R) {
                float ty = 0.4f * qSqrt(R*R - tx*tx - tz*tz);
                targetPosition = {tx, ty, tz};
                targetNormal = targetPosition;
            } else {
                targetPosition = {tx, -3, tz};
                targetNormal = {0, 1, 0};
            }

            if (ix == 0 || iy == 0 || ix == iw-1 || iy == ih-1) {
                int iix = qMin(iw-2, qMax(1, ix));
                int iiy = qMin(ih-2, qMax(1, iy));
                x = iix * wf - dw;
                z = iiy * hf - dh;
                y = -3.0;
                targetPosition.setY(-3);
            }

            if (iy >= ih-2)
                normal = {0, 0, 1};
            else if (iy <= 1)
                normal = {0, 0, -1};

            m_positions.append({x, y, z});
            m_normals.append(normal.normalized());

            m_targetPositions.append(targetPosition);
            m_targetNormals.append(targetNormal.normalized());
        }
    }

    for (int ix = 0; ix < iw - 1; ++ix) {
        for (int iy = 0; iy < ih - 1; ++iy) {
            int idx = ix + iy * ih;
            m_indexes << idx << idx + iw << idx + iw + 1
                      << idx << idx + iw + 1 << idx + 1;
        }
    }
}

