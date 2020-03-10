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

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

/*!
    \qmltype Geometry
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \instantiates QQuick3DGeometry
    \brief An Abstract base type for custom geometry.

    The Geometry can be used to specify custom geometry used with Qt Quick 3D.
    The custom geometry should be implemented with C++ and registered to QML.

    The geometry is identified with unique name, which is used by the engine to distinguish
    different models. Instances of same custom geometry type with different parameters should
    specify different unique id. The name can be used with \l {Model::source}{model source} to
    share the same geometry with different models.

    \code

    import QtQuick3D.Helpers 1.15

    Model {
        id: gridModel
        geometry: GridGeometry {
            name: "grid"
        }
        materials: [
            DefaultMaterial {
                emissiveColor: "white"
                lighting: DefaultMaterial.NoLighting
            }
        ]
    }
    \endcode

    \sa Model
*/

/*!
    \qmlproperty string Geometry::name

    Unique name identifying the geometry.
*/

/*!
    \class QQuick3DGeometry
    \inmodule QtQuick3D
    \inherits QQuick3DObject
    \since 5.15
    \brief Base class for defining custom geometry.

    The QQuick3DGeometry can be used to specify custom geometry used with Qt Quick 3D.

    The user should inherit this class and specify it's own properties, which can then be used
    from QML code. The user then should use these properties to construct the geometry and set
    it for the QQuick3DGeometry, which then uploads it to the Qt Quick3D engine.

    Example implementation:

    \code
    class CustomGeometry : public QQuick3DGeometry
    {
        Q_OBJECT
        ... properties ...

    public:
        CustomGeometry();

        void setProperty(...)
        {
            ...
            rebuildGeometry();
        }

    private:
        void rebuildGeometry()
        {
            QByteArray vertices;
            QByteArray indices;
            fillGeometry(vertices, indices);
            setPrimitiveType(Lines);
            setVertexBuffer(vertices);
            setIndexBuffer(indices);
            setStride(sizeof(QVector3D));
            setBounds(...);
            addAttrubute(PositionSemantic, 0, F32Type);
        }
    };
    \endcode

    This class can then be registered as a QML type and used with \l {QtQuick3D::Model}{Model}.
    \code
    qmlRegisterType<CustomGeometry>("Example", 1, 0, "CustomGeometry");
    \endcode
    \code

    import Example 1.0

    Model {
        id: customModel
        geometry: CustomGeometry {
        }
    }
    \endcode
*/

QT_BEGIN_NAMESPACE

QQuick3DGeometryPrivate::QQuick3DGeometryPrivate()
    : QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Geometry)
{

}

QQuick3DGeometry::QQuick3DGeometry(QQuick3DObject *parent)
    : QQuick3DObject(*new QQuick3DGeometryPrivate, parent)
{

}

QQuick3DGeometry::~QQuick3DGeometry()
{

}

/*!
    \property QQuick3DGeometry::name

    Unique name identifying the geometry. This becomes the source path for the geometry.
    If multiple instances from the same geometry class are used, each of them must have
    their own unique name. Otherwise, geometry with same name will override the others.
    Geometry can be shared either by setting the geometry parameter for a model or using the
    name of the geometry as source parameter for the model.
*/
QString QQuick3DGeometry::name() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_name;
}

/*!
    Returns the vertex buffer data.
*/
QByteArray QQuick3DGeometry::vertexBuffer() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_vertexBuffer;
}

/*!
    Returns the index buffer data.
*/
QByteArray QQuick3DGeometry::indexBuffer() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_indexBuffer;
}

/*!
    Returns the attribute count.
*/
int QQuick3DGeometry::attributeCount() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_attributeCount;
}

/*!
    Returns an attribute at \a index
*/
QQuick3DGeometry::Attribute QQuick3DGeometry::attribute(int index) const
{
    const Q_D(QQuick3DGeometry);
    return d->m_attributes[index];
}

/*!
    Returns the primitive type. The default is \c Triangles.

    \value Unknown The primitive type is not set.
    \value Points The primitives are points.
    \value LineStrip The primitives are lines in a strip.
    \value Lines The primitives are lines in a list.
    \value TriangleStrip The primitives are triangles in a strip.
    \value TriangleFan The primitives are triangles in a fan.
    \value Triangles The primitives are triangles in a list.
*/
QQuick3DGeometry::PrimitiveType QQuick3DGeometry::primitiveType() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_primitiveType;
}

/*!
    Returns the minimum bound coordinate.
*/
QVector3D QQuick3DGeometry::boundsMin() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_min;
}

/*!
    Returns the maximum bound coordinate.
*/
QVector3D QQuick3DGeometry::boundsMax() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_max;
}

/*!
    Returns the byte stride of the vertex buffer.
*/
int QQuick3DGeometry::stride() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_stride;
}

void QQuick3DGeometry::markAllDirty()
{
    Q_D(QQuick3DGeometry);
    d->m_nameChanged = true;
    QQuick3DObject::markAllDirty();
}

void QQuick3DGeometry::setName(const QString &name)
{
    Q_D(QQuick3DGeometry);
    if (name != d->m_name) {
        d->m_nameChanged = true;
        d->m_name = name;
        Q_EMIT nameChanged();
        update();
    }
}

/*!
    Sets the vertex buffer \a data. The buffer should hold all the vertex data
    packed in the array described by the attributes.
*/
void QQuick3DGeometry::setVertexData(const QByteArray &data)
{
    Q_D(QQuick3DGeometry);
    d->m_vertexBuffer = data;
    d->m_geometryChanged = true;
}

/*!
    Sets the index buffer \a data. If the index buffer is not set, the vertex buffer
    is used as is for the vertices.
*/
void QQuick3DGeometry::setIndexData(const QByteArray &data)
{
    Q_D(QQuick3DGeometry);
    d->m_indexBuffer = data;
    d->m_geometryChanged = true;
}

/*!
    Sets the byte \a stride of the vertex.
*/
void QQuick3DGeometry::setStride(int stride)
{
    Q_D(QQuick3DGeometry);
    if (stride != d->m_stride) {
        d->m_stride = stride;
        d->m_geometryChanged = true;
    }
}

/*!
    Sets the bounds of the geometry with \a min and \a max point.
*/
void QQuick3DGeometry::setBounds(const QVector3D &min, const QVector3D &max)
{
    Q_D(QQuick3DGeometry);
    d->m_max = max;
    d->m_min = min;
    d->m_geometryBoundsChanged = true;
}

/*!
    Sets the primitive \a type.

    \value UnknownType The primitive type is not set.
    \value Points The primitives are points.
    \value LineStrip The primitives are lines in a strip.
    \value Lines The primitives are lines in a list.
    \value TriangleStrip The primitives are triangles in a strip.
    \value TriangleFan The primitives are triangles in a fan.
    \value Triangles The primitives are triangles in a list.
*/
void QQuick3DGeometry::setPrimitiveType(PrimitiveType type)
{
    Q_D(QQuick3DGeometry);
    if (d->m_primitiveType != type) {
        d->m_primitiveType = type;
        d->m_geometryChanged = true;
    }
}

/*!
    Adds vertex attribute description. Each attribute has a \a semantic, which specifies
    the usage of the attribute and the number of components it has, an \a offset from the
    beginning to the vertex to the attribute location inside a vertex and a \a componentType
    specifying the datatype and size of the attribute.

    The semantic can be one of the following:

    \value UnknownSemantic The semantic is not set.
    \value IndexSemantic The attribute is an index.
    \value PositionSemantic The attribute is a position.
    \value NormalSemantic The attribute is a normal vector.
    \value TexCoordSemantic The attribute is a texture coordinate.
    \value TangentSemantic The attribute is a tangent vector.
    \value BinormalSemantic The attribute is a binormal vector.

    The component type can be one of the following:

    \value DefaultType The attribute uses default type depending on the semantic.
    \value U16Type The attribute is an unsigned 16-bit integer.
    \value U32Type The attribute is an unsigned 32-bit integer. This is the default for IndexSemantic.
    \value F32Type The attribute is a single-precision float. This is the default for most semantics.
*/
void QQuick3DGeometry::addAttribute(Attribute::Semantic semantic, int offset,
                  Attribute::ComponentType componentType)
{
    Q_D(QQuick3DGeometry);
    if (d->m_attributeCount >= QQuick3DGeometryPrivate::MAX_ATTRIBUTE_COUNT)
        return;
    d->m_attributes[d->m_attributeCount].semantic = semantic;
    d->m_attributes[d->m_attributeCount].offset = offset;
    d->m_attributes[d->m_attributeCount].componentType = componentType;
    d->m_attributeCount++;
    d->m_geometryChanged = true;
}

/*!
    Adds vertex attribute description. Each attribute has a semantic, which specifies
    the usage of the attribute and the number of components it has, an offset from the
    beginning to the vertex to the attribute location inside a vertex and a componentType
    specifying the datatype and size of the attribute.
*/
void QQuick3DGeometry::addAttribute(const Attribute &attribute)
{
    Q_D(QQuick3DGeometry);
    if (d->m_attributeCount >= QQuick3DGeometryPrivate::MAX_ATTRIBUTE_COUNT)
        return;
    d->m_attributes[d->m_attributeCount++] = attribute;
    d->m_geometryChanged = true;
}

/*!
    Clears previously set vertex- and index data as well as attributes.
*/
void QQuick3DGeometry::clear()
{
    Q_D(QQuick3DGeometry);
    d->m_vertexBuffer.clear();
    d->m_indexBuffer.clear();
    d->m_attributeCount = 0;
    d->m_primitiveType = PrimitiveType::Unknown;
    d->m_geometryChanged = true;
}

QSSGRenderGraphObject *QQuick3DGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    Q_D(QQuick3DGeometry);
    if (!node) {
        markAllDirty();
        node = new QSSGRenderGeometry();
        emit geometryNodeDirty();
    }

    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);
    if (d->m_nameChanged) {
        geometry->setPath(d->m_name);
        d->m_nameChanged = false;
    }
    if (d->m_geometryChanged) {
        geometry->setBounds(d->m_min, d->m_max);
        geometry->setStride(d->m_stride);
        geometry->setIndexData(d->m_indexBuffer);
        geometry->setVertexData(d->m_vertexBuffer);
        geometry->setPrimitiveType(QSSGRenderGeometry::PrimitiveType(d->m_primitiveType));
        geometry->clearAttributes();
        for (int i = 0; i < d->m_attributeCount; ++i) {
            geometry->addAttribute(QSSGRenderGeometry::Attribute::Semantic(d->m_attributes[i].semantic),
                                   d->m_attributes[i].offset,
                                   QSSGRenderGeometry::Attribute::ComponentType(d->m_attributes[i].componentType));
        }
        d->m_geometryChanged = false;
    }
    if (d->m_geometryBoundsChanged) {
        geometry->setBounds(d->m_min, d->m_max);
        emit geometryNodeDirty();
        d->m_geometryBoundsChanged = false;
    }

    return node;
}

QT_END_NAMESPACE
