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
//    "uniforms": [ { "type": "mat44", "name": "qt_viewProjectionMatrix" },
//                  { "type": "mat4", "name": "qt_viewMatrix" },
//                  { "type": "vec2", "name": "qt_cameraProperties" },
//                  { "type": "vec3", "name": "qt_cameraPosition", "condition": "!SSAO_CUSTOM_MATERIAL_GLSLLIB" }
//    ]
// }*/
// #endif // QQ3D_SHADER_META

namespace QSSGRenderShaderMetadata {

const char *shaderMetaStart() { return "#ifdef QQ3D_SHADER_META"; }
const char *shaderMetaEnd() { return "#endif"; }

Uniform::Condition Uniform::conditionFromString(const QString &condition)
{
    if (condition.isEmpty())
        return Uniform::Condition::None;

    if (condition.at(0) == QChar::fromLatin1('!'))
        return Uniform::Negated;

    return Uniform::Regular;
}

QSSGShaderGeneratorStage InputOutput::stageFromString(const QString &stage)
{
    if (stage == QLatin1String("vertex")) {
        return QSSGShaderGeneratorStage::Vertex;
    } else if (stage == QLatin1String("fragment"))
        return QSSGShaderGeneratorStage::Fragment;
    else {
        qWarning("Unknown stage in shader metadata: %s, assuming vertex", qPrintable(stage));
        return QSSGShaderGeneratorStage::Vertex;
    }
}

ShaderMetaData getShaderMetaData(const QByteArray &data)
{
    ShaderMetaData result;
    if (data.isEmpty())
        return result;

    int jsonStart = 0, jsonEnd = 0;
    for ( ; ; ) {
        jsonStart = data.indexOf(shaderMetaStart(), jsonEnd);
        if (jsonStart)
            jsonEnd = data.indexOf(shaderMetaEnd(), jsonStart);

        if (jsonEnd) // adjust start position
            jsonStart += int(strlen(shaderMetaStart()));

        if (jsonStart <= 0 || jsonEnd <= 0)
            break;

        const int size = jsonEnd - jsonStart;
        // /*{"inputs":[]}*/ => 17
        if (size < 17) {
            qWarning("Shader metadata section found, but content to small to be valid!");
            break;
        }

        QByteArray jsonData = data.mid(jsonStart, size).trimmed();
        if (!jsonData.startsWith(QByteArrayLiteral("/*{"))) {
            qWarning("Missing /*{ prefix");
            break;
        }
        if (!jsonData.endsWith(QByteArrayLiteral("}*/"))) {
            qWarning("Missing }*/ suffix");
            break;
        }
        jsonData = jsonData.mid(2, jsonData.count() - 4);

        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(jsonData, &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "Shader metadata parse error at offset: " << error.offset;
            break;
        }

        static const auto toUniform = [](const QJsonObject &uObj) {
            Uniform uniform;
            if (!uObj.isEmpty()) {
                const auto type = uObj.find(QLatin1String("type"));
                uniform.type = type.value().toString().toLatin1();
                uniform.name = uObj.find(QLatin1String("name"))->toString().toLatin1();
                const auto conditionString = uObj.find(QLatin1String("condition"))->toString();
                uniform.condition = Uniform::conditionFromString(conditionString);
                if (uniform.condition == Uniform::Negated)
                    uniform.conditionName = conditionString.mid(1).toLatin1();
                else if (uniform.condition == Uniform::Regular)
                    uniform.conditionName = conditionString.toLatin1();
            }
            return uniform;
        };

        static const auto toInputOutput = [](const QJsonObject &uObj) {
            InputOutput inOutVar;
            if (!uObj.isEmpty()) {
                const auto type = uObj.find(QLatin1String("type"));
                inOutVar.type = type.value().toString().toLatin1();
                inOutVar.name = uObj.find(QLatin1String("name"))->toString().toLatin1();
                inOutVar.stage = InputOutput::stageFromString(uObj.find(QLatin1String("stage"))->toString());
            }
            return inOutVar;
        };

        const QJsonObject obj = doc.object();

        const auto uniforms = obj.constFind(QLatin1String("uniforms"));
        // Check if it's an array or a single object
        if (uniforms->type() == QJsonValue::Array) {
            const auto uniformArray = uniforms.value().toArray();
            for (const auto it : uniformArray) {
                if (!it.isObject())
                    continue;

                const QJsonObject obj = it.toObject();
                const auto uniform = toUniform(obj);
                if (!uniform.type.isEmpty() && !uniform.name.isEmpty()) {
                    result.uniforms.push_back(uniform);
                } else {
                    qWarning("Invalid uniform (%s %s), skipping",
                             qPrintable(obj.find(QLatin1String("type")).value().toString()),
                             qPrintable(obj.find(QLatin1String("name")).value().toString()));
                }
            }
        } else if (uniforms->type() == QJsonValue::Object) {
            const auto uniform = toUniform(uniforms.value().toObject());
            if (!uniform.type.isEmpty() && !uniform.name.isEmpty())
                result.uniforms.push_back(uniform);
            else
                qWarning("Invalid uniform, skipping");
        }

        const auto inputs = obj.constFind(QLatin1String("inputs"));
        if (inputs->type() == QJsonValue::Array) {
            for (const auto it : inputs.value().toArray()) {
                if (!it.isObject())
                    continue;
                const auto inOutVar = toInputOutput(it.toObject());
                if (!inOutVar.type.isEmpty() && !inOutVar.name.isEmpty())
                    result.inputs.push_back(inOutVar);
                else
                    qWarning("Invalid input variable, skipping");
            }
        } else if (inputs->type() == QJsonValue::Object) {
            const QJsonObject obj = inputs.value().toObject();
            const auto inOutVar = toInputOutput(obj);
            if (!inOutVar.type.isEmpty() && !inOutVar.name.isEmpty()) {
                result.inputs.push_back(inOutVar);
            } else {
                qWarning("Invalid input variable (%s %s), skipping",
                         qPrintable(obj.find(QLatin1String("type")).value().toString()),
                         qPrintable(obj.find(QLatin1String("name")).value().toString()));
            }
        }

        const auto outputs = obj.constFind(QLatin1String("outputs"));
        if (outputs->type() == QJsonValue::Array) {
            for (const auto it : outputs.value().toArray()) {
                if (!it.isObject())
                    continue;
                const auto inOutVar = toInputOutput(it.toObject());
                if (!inOutVar.type.isEmpty() && !inOutVar.name.isEmpty())
                    result.outputs.push_back(inOutVar);
                else
                    qWarning("Invalid output variable, skipping");
            }
        } else if (outputs->type() == QJsonValue::Object) {
            const QJsonObject obj = outputs.value().toObject();
            const auto inOutVar = toInputOutput(obj);
            if (!inOutVar.type.isEmpty() && !inOutVar.name.isEmpty()) {
                result.outputs.push_back(inOutVar);
            } else {
                qWarning("Invalid output variable (%s %s), skipping",
                         qPrintable(obj.find(QLatin1String("type")).value().toString()),
                         qPrintable(obj.find(QLatin1String("name")).value().toString()));
            }
        }
    }

    return result;
}

} // namespace
