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

#include "qssgrendershadermetadata_p.h"

#include <QPair>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <QtDebug>

// Example snippet based on viewProperties.glsllib
// The comment in the ifdefed block is necessary to keep weird shader compilers (Vivante) happy.

// #ifdef QQ3D_SHADER_META
// /*{
//    "uniforms": [ { "type": "mat44", "name": "viewProjectionMatrix" },
//                  { "type": "mat4", "name": "viewMatrix" },
//                  { "type": "vec2", "name": "cameraProperties" },
//                  { "type": "vec3", "name": "cameraPosition", "condition": "!SSAO_CUSTOM_MATERIAL_GLSLLIB" }
//    ]
// }*/
// #endif // QQ3D_SHADER_META

namespace QSSGRenderShaderMetadata {

const char *shaderMetaStart() { return "#ifdef QQ3D_SHADER_META"; }
const char *shaderMetaEnd() { return "#endif"; }

int Uniform::typeFromString(const QString &type)
{
    static const auto vecType = [](char c)
    {
        switch (c) {
        case '2': return Uniform::Vec2;
        case '3': return Uniform::Vec3;
        case '4': return Uniform::Vec4;
        }

        Q_ASSERT(0);
        return Uniform::Invalid;
    };

    static const auto matType = [](const QStringRef &ref) {
        if (ref.length() == 1) {
            const int value = ref.at(0).toLatin1() - 48;
            if (value >= 0 && value <= 4)
                return Uniform::Type::Mat + value;
        } else if (ref.length() == 2) {
            const int n = ref.at(0).toLatin1() - 48;
            const int m = ref.at(1).toLatin1() - 48;
            if (n >= 0 && n <= 4 && m >= 0 && m <= 4)
                return Uniform::Type::Mat + (n << 3) + m;
        }

        return int(Uniform::Invalid);
    };

    switch (type.at(0).toLatin1()) {
    case 'b':
        if (type == QLatin1String("bool"))
            return Uniform::Boolean;
        if (type.startsWith(QLatin1String("bvec")))
            return (Uniform::Boolean | vecType(type.at(4).toLatin1()));
        break;
    case 'i':
        if (type == QLatin1String("int"))
            return Uniform::Int;
        if (type.startsWith(QLatin1String("ivec")))
            return (Uniform::Int | vecType(type.at(4).toLatin1()));
        break;
    case 'u':
        if (type == QLatin1String("uint"))
            return Uniform::Uint;
        if (type.startsWith(QLatin1String("uvec")))
            return (Uniform::Uint | vecType(type.at(4).toLatin1()));
        break;
    case 'f':
        if (type == QLatin1String("float"))
            return Uniform::Float;
        break;
    case 'v':
        if (type.startsWith(QLatin1String("vec")))
            return (Uniform::Float | vecType(type.at(3).toLatin1()));
        break;
    case 'd':
        if (type == QLatin1String("double"))
            return Uniform::Double;
        if (type == QLatin1String("dvec"))
            return (Uniform::Double | vecType(type.at(4).toLatin1()));
        break;
    case 'm':
        if (type.startsWith(QLatin1String("mat")))
            return matType(type.midRef(3));
        break;
    case 's':
        if (type == QLatin1String("sampler2D"))
            return Uniform::Sampler;
        break;
    }

    // TODO: Samplers and Matrices etc.

    return Uniform::Invalid;
}

Uniform::Condition Uniform::conditionFromString(const QString &condition)
{
    if (condition.isEmpty())
        return Uniform::Condition::None;

    if (condition.at(0) == '!')
        return Uniform::Negated;

    return Uniform::Regular;
}


UniformList getShaderMetaData(const QByteArray &data)
{
    UniformList uniformList;

    int jsonStart = 0, jsonEnd = 0;
    if (data.size()) {
        jsonStart = data.indexOf(shaderMetaStart());
        if (jsonStart)
            jsonEnd = data.indexOf(shaderMetaEnd(), jsonStart) - 1;

        if (jsonEnd) // adjust start position
            jsonStart += int(strlen(shaderMetaStart())) + 1;
    }

    if (jsonStart <= 0 || jsonEnd <= 0)
        return uniformList;

    const int size = jsonEnd - jsonStart;
    // /*{"uniforms":{"name":"x","type":"y"}}*/ => 40
    if (size < 40) {
        qWarning("Meta-data section found, but content to small to be valid!");
        return uniformList;
    }

    QByteArray jsonData = data.mid(jsonStart, size);
    if (!jsonData.startsWith(QByteArrayLiteral("/*{"))) {
        qWarning("Missing /*{ prefix");
        return uniformList;
    }
    if (!jsonData.endsWith(QByteArrayLiteral("}*/"))) {
        qWarning("Missing }*/ suffix");
        return uniformList;
    }
    jsonData = jsonData.mid(2, jsonData.count() - 4);

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Parsing error at offset: " << error.offset;
        return uniformList;
    }

    static const auto toUniform = [](const QJsonObject &uObj) {
        Uniform uniform;
        if (!uObj.isEmpty()) {
            const auto type = uObj.find(QLatin1String("type"));
            uniform.type = Uniform::typeFromString(type.value().toString());
            if (uniform.type >= 0) {
                uniform.name = uObj.find(QLatin1String("name"))->toString().toLatin1();
                const auto conditionString = uObj.find(QLatin1String("condition"))->toString();
                uniform.condition = Uniform::conditionFromString(conditionString);
                if (uniform.condition == Uniform::Negated)
                    uniform.conditionName = conditionString.mid(1).toLatin1();
                else if (uniform.condition == Uniform::Regular)
                    uniform.conditionName = conditionString.toLatin1();
            }
        }

        return uniform;
    };

    const auto obj = doc.object();
    const auto uniforms = obj.constFind(QLatin1String("uniforms")); // uniforms is currently the only supported type
    // Check if it's an array or a single object (uniform)
    if (uniforms->type() == QJsonValue::Array) {
        const auto uniformArray = uniforms.value().toArray();
        for (const auto it : uniformArray) {
            if (!it.isObject())
                continue;

            const auto uniform = toUniform(it.toObject());
            if (uniform.type != Uniform::Type::Invalid)
                uniformList.push_back(uniform);
            else
                qWarning("Invalid uniform, skipping!");

        }
    } else if (uniforms->type() == QJsonValue::Object) {
        const auto uniform = toUniform(uniforms.value().toObject());
        if (uniform.type != Uniform::Type::Invalid)
            uniformList.push_back(uniform);
        else
            qWarning("Invalid uniform, skipping!");
    } else {
        qWarning("Unsupported type in meta type in FOO");
    }

    return uniformList;
}

} // namespace
