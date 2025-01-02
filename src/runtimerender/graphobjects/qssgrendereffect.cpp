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
    resetCommands();
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

static const char *effect_fragment_main_with_tonemapping =
        "#include \"tonemapping.glsllib\"\n"
        "void main()\n"
        "{\n"
        "    qt_customMain();\n"
        "    fragOutput = qt_tonemap(fragOutput);\n"
        "}\n";

void QSSGRenderEffect::finalizeShaders(const QSSGRenderLayer &layer, QSSGRenderContextInterface *renderContext)
{
    Q_UNUSED(layer);

    // this is called on every frame, so do nothing if there are no changes
    if (!shaderPrepData.valid)
        return;

    QRhi *rhi = renderContext->rhiContext()->rhi();

    for (int i = 0, ie = shaderPrepData.passes.size(); i != ie; ++i) {
        const ShaderPrepPassData &pass(shaderPrepData.passes[i]);

        // The fragment shader of the last pass of the last effect may need to
        // perform the built-in tonemapping.
        const bool isLastEffect = m_nextEffect == nullptr;
        const bool isLastPass = i == ie - 1;
        const bool shouldTonemapIfEnabled = isLastEffect && isLastPass;

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
            if (shouldTonemapIfEnabled)
                code.append(effect_fragment_main_with_tonemapping);
            else
                code.append(effect_fragment_main);
            completeFragmentShader = code;
            sourceCodeForHash += code;
        }

        QByteArray shaderPathKey = pass.shaderPathKeyPrefix;
        shaderPathKey.append(':' + QCryptographicHash::hash(sourceCodeForHash, QCryptographicHash::Algorithm::Sha1).toHex());

        // QSSGRhiEffectSystem will vary the vertex shader code based on this
        // flag from the QRhi. It is therefore important to capture this in the
        // cache key as well.
        shaderPathKey.append(':' + QByteArray::number(rhi->isYUpInFramebuffer() ? 1 : 0));

        if (shouldTonemapIfEnabled) {
            // This does not always mean there will be tonemapping: if the mode
            // is TonemapModeNone, then no extra feature defines are set, and
            // so qt_tonemap() in the shader will not alter the color.
            const QSSGRenderLayer::TonemapMode tonemapMode = layer.tonemapMode;
            shaderPathKey.append(':' + QByteArray::number(int(tonemapMode)));
            QSSGRenderer::setTonemapFeatures(features, tonemapMode);
        }

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
        delete commands[pass.bindShaderCmdIndex].command;
        commands[pass.bindShaderCmdIndex] = { new QSSGBindShader(shaderPathKey), true };
    }

    shaderPrepData.valid = false;
}

void QSSGRenderEffect::resetCommands()
{
    for (const Command &cmd : commands) {
        if (cmd.own)
            delete cmd.command;
    }
    commands.clear();
    shaderPrepData.passes.clear();
}

QT_END_NAMESPACE
