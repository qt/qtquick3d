// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

QT_BEGIN_NAMESPACE

QSSGRenderEffect::QSSGRenderEffect() : QSSGRenderGraphObject(Type::Effect) {}

QSSGRenderEffect::~QSSGRenderEffect()
{
    qDeleteAll(commands);
}

void QSSGRenderEffect::markDirty()
{
    flags |= FlagT(Flags::Dirty);
}

void QSSGRenderEffect::clearDirty()
{
    flags &= ~FlagT(Flags::Dirty);
}

// Suffix snippets added to the end of the shader strings. These are appended
// after processing so it must be valid GLSL as-is, no more magic keywords.

static const char *effect_vertex_main_pre =
        "void main()\n"
        "{\n"
        "    qt_inputUV = attr_uv;\n"
        "    qt_textureUV = qt_effectTextureMapUV(attr_uv);\n"
        "    vec4 qt_vertPosition = vec4(attr_pos, 1.0);\n"
        "    qt_customMain(qt_vertPosition.xyz);\n";

static const char *effect_vertex_main_position =
        "    gl_Position = qt_modelViewProjection * qt_vertPosition;\n";

static const char *effect_vertex_main_post =
        "}\n";

static const char *effect_fragment_main =
        "void main()\n"
        "{\n"
        "    qt_customMain();\n"
        "}\n";

void QSSGRenderEffect::finalizeShaders(const QSSGRenderLayer &layer, QSSGRenderContextInterface *renderContext)
{
    Q_UNUSED(layer);

    // this is called on every frame, so do nothing if there are no changes
    if (!shaderPrepData.valid)
        return;

    for (int i = 0, ie = shaderPrepData.passes.count(); i != ie; ++i) {
        const ShaderPrepPassData &pass(shaderPrepData.passes[i]);

        QSSGShaderFeatures features;
        QByteArray completeVertexShader;
        QByteArray completeFragmentShader;
        QByteArray sourceCodeForHash;
        if (!pass.vertexShaderCode.isEmpty()) {
            QByteArray code = pass.vertexShaderCode;
            // add the real main(), with or without assigning gl_Position at the end
            code.append(effect_vertex_main_pre);
            if (!pass.vertexMetaData.flags.testFlag(QSSGCustomShaderMetaData::OverridesPosition))
                code.append(effect_vertex_main_position);
            code.append(effect_vertex_main_post);
            completeVertexShader = code;
            sourceCodeForHash += code;
        }
        if (!pass.fragmentShaderCode.isEmpty()) {
            QByteArray code = pass.fragmentShaderCode;
            code.append(effect_fragment_main);
            completeFragmentShader = code;
            sourceCodeForHash += code;
        }

        QByteArray shaderPathKey = pass.shaderPathKeyPrefix;
        shaderPathKey.append(':' + QCryptographicHash::hash(sourceCodeForHash, QCryptographicHash::Algorithm::Sha1).toHex());

        // Now that the final shaderPathKey is known, store the source and
        // related data; it will be retrieved later by the QSSGRhiEffectSystem.
        if (!completeVertexShader.isEmpty()) {
            renderContext->shaderLibraryManager()->setShaderSource(shaderPathKey,
                                                                   QSSGShaderCache::ShaderType::Vertex,
                                                                   completeVertexShader,
                                                                   pass.vertexMetaData);
        }
        if (!completeFragmentShader.isEmpty()) {
            QSSGCustomShaderMetaData metaData = pass.fragmentMetaData;
            metaData.features = features;
            renderContext->shaderLibraryManager()->setShaderSource(shaderPathKey,
                                                                   QSSGShaderCache::ShaderType::Fragment,
                                                                   completeFragmentShader,
                                                                   metaData);
        }

        // and update the command
        delete commands[pass.bindShaderCmdIndex];
        commands[pass.bindShaderCmdIndex] = new QSSGBindShader(shaderPathKey);
    }

    shaderPrepData.valid = false;
}

QT_END_NAMESPACE
