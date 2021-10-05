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

#ifndef CUSTOMMATERIAL_H
#define CUSTOMMATERIAL_H

#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>

QT_BEGIN_NAMESPACE

class CustomMaterial
{
public:
    struct Uniform
    {
        enum class Type
        {
            Bool,
            Int,
            Float,
            Vec2,
            Vec3,
            Vec4,
            Mat44,
            Sampler,
            Last
        };
        union {
            bool b;
            int i;
            float f;
            QVector2D vec2;
            QVector3D vec3;
            QVector4D vec4;
            quintptr d;
        };
        Type type;
        QByteArray name;
        QMatrix4x4 m44;
        QString imagePath;
    };
    using UniformTable = QList<Uniform>;
    using TextureStore = QList<QImage *>;

    struct Properties
    {
        QQuick3DCustomMaterial::CullMode cullMode { QQuick3DCustomMaterial::CullMode::BackFaceCulling };
        QQuick3DCustomMaterial::DepthDrawMode depthDrawMode { QQuick3DCustomMaterial::DepthDrawMode::OpaqueOnlyDepthDraw };
        QQuick3DCustomMaterial::ShadingMode shadingMode { QQuick3DCustomMaterial::ShadingMode::Shaded };
        QQuick3DCustomMaterial::BlendMode sourceBlend { QQuick3DCustomMaterial::BlendMode::NoBlend };
        QQuick3DCustomMaterial::BlendMode destinationBlend { QQuick3DCustomMaterial::BlendMode::NoBlend };
    } properties;

    struct Shaders
    {
        QUrl vert;
        QUrl frag;
    } shaders;

    static void setUniform(QSSGSceneDesc::Material &material, const Uniform &uniform);
    QPointer<QQuick3DCustomMaterial> create(QQuick3DObject &parent, const UniformTable &uniforms, const Properties &properties, const Shaders &shaders);
    bool isValid() const;
    friend QTextStream &operator<<(QTextStream &stream, const CustomMaterial &material)
    {
        using namespace QSSGSceneDesc;
        const auto &scene = material.scene;
        if (auto material = scene.root) {
            Q_ASSERT(material->runtimeType == Material::RuntimeType::CustomMaterial);
            writeQmlComponent(static_cast<const QSSGSceneDesc::Material &>(*material), stream);
        }
        return stream;
    }

    friend QDataStream &operator<<(QDataStream &stream, const CustomMaterial::Uniform &uniform) { return writeToDataStream(stream, uniform); }
    friend QDataStream &operator>>(QDataStream &stream, CustomMaterial::Uniform &uniform) { return readFromDataStream(stream, uniform); }

private:
    static QDataStream &readFromDataStream(QDataStream &stream, Uniform &uniform);
    static QDataStream &writeToDataStream(QDataStream &stream, const Uniform &uniform);
    static void writeQmlComponent(const QSSGSceneDesc::Material &material, QTextStream &stream);
    QSSGSceneDesc::Scene scene;
};

QT_END_NAMESPACE

#endif // CUSTOMMATERIAL_H
