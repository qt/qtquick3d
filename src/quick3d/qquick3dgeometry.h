/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef Q_QUICK3D_GEOMETRY_H
#define Q_QUICK3D_GEOMETRY_H

#include <QtQuick3D/qquick3dobject.h>

QT_BEGIN_NAMESPACE

class QQuick3DGeometryPrivate;

class Q_QUICK3D_EXPORT QQuick3DGeometry : public QQuick3DObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuick3DGeometry)

    QML_NAMED_ELEMENT(Geometry)
    QML_UNCREATABLE("Geometry is Abstract")
    QML_ADDED_IN_VERSION(1, 14)
public:
    explicit QQuick3DGeometry(QQuick3DObject *parent = nullptr);
    ~QQuick3DGeometry() override;

    enum class PrimitiveType {
        Points,
        LineStrip,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles
    };

    struct Attribute {
        enum Semantic {
            IndexSemantic,
            PositionSemantic, // attr_pos
            NormalSemantic,   // attr_norm
            TexCoordSemantic, // attr_uv0
            TangentSemantic,  // attr_textan
            BinormalSemantic, // attr_binormal
            JointSemantic,    // attr_joints
            WeightSemantic,   // attr_weights
            ColorSemantic     // attr_color
        };
        enum ComponentType {
            U16Type,
            U32Type,
            I32Type,
            F32Type
        };
        Semantic semantic = PositionSemantic;
        int offset = -1;
        ComponentType componentType = F32Type;
    };

    QByteArray vertexData() const;
    QByteArray indexData() const;
    int attributeCount() const;
    Attribute attribute(int index) const;
    PrimitiveType primitiveType() const;
    QVector3D boundsMin() const;
    QVector3D boundsMax() const;
    int stride() const;

    void setVertexData(const QByteArray &data);
    void setVertexData(int offset, const QByteArray &data);
    void setIndexData(const QByteArray &data);
    void setIndexData(int offset, const QByteArray &data);
    void setStride(int stride);
    void setBounds(const QVector3D &min, const QVector3D &max);
    void setPrimitiveType(PrimitiveType type);

    void addAttribute(Attribute::Semantic semantic, int offset,
                      Attribute::ComponentType componentType);
    void addAttribute(const Attribute &att);

    void clear();

Q_SIGNALS:
    void geometryNodeDirty();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
};

QT_END_NAMESPACE

#endif // Q_QUICK3D_GEOMETRY_H
