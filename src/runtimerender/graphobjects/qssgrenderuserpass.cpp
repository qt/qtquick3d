// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qssgrenderuserpass_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadertokenizer_p.h>
#include <QtCore/qcryptographichash.h>

#include <ssg/qssgrendercontextcore.h>

#include <iterator>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcRenderUserPass, "qt.quick3d.runtimerender.renderuserpass")

namespace
{

enum class AugmentMacro : size_t {
    BASE_COLOR,
    ROUGHNESS,
    METALNESS,
    DIFFUSE_LIGHT,
    SPECULAR_LIGHT,
    EMISSIVE_LIGHT,
    WORLD_NORMAL,
    WORLD_TANGENT,
    WORLD_BINORMAL,
    F0,
    F90
};

static constexpr QSSGUserShaderAugmentation::FeatureFlags featureForMacro(AugmentMacro macro)
{
    switch (macro) {
    case AugmentMacro::BASE_COLOR:
        return QSSGUserShaderAugmentation::FeatureFlags::BaseColor;
    case AugmentMacro::ROUGHNESS:
        return QSSGUserShaderAugmentation::FeatureFlags::Roughness;
    case AugmentMacro::METALNESS:
        return QSSGUserShaderAugmentation::FeatureFlags::Metalness;
    case AugmentMacro::DIFFUSE_LIGHT:
        return QSSGUserShaderAugmentation::FeatureFlags::DiffuseLight;
    case AugmentMacro::SPECULAR_LIGHT:
        return QSSGUserShaderAugmentation::FeatureFlags::SpecularLight;
    case AugmentMacro::EMISSIVE_LIGHT:
        return QSSGUserShaderAugmentation::FeatureFlags::EmissiveLight;
    case AugmentMacro::WORLD_NORMAL:
        return QSSGUserShaderAugmentation::FeatureFlags::WorldNormal;
    case AugmentMacro::WORLD_TANGENT:
        return QSSGUserShaderAugmentation::FeatureFlags::WorldTangent;
    case AugmentMacro::WORLD_BINORMAL:
        return QSSGUserShaderAugmentation::FeatureFlags::WorldBinormal;
    case AugmentMacro::F0:
        return QSSGUserShaderAugmentation::FeatureFlags::F0;
    case AugmentMacro::F90:
        return QSSGUserShaderAugmentation::FeatureFlags::F90;
    }

    Q_UNREACHABLE_RETURN(QSSGUserShaderAugmentation::FeatureFlags::None);
}

constexpr size_t QSSGMaxMacroLength = 16;
constexpr size_t QSSGMaxReplacementLength = 32;

struct MacroPair {
    const char macroName[QSSGMaxMacroLength];
    const char replacement[QSSGMaxReplacementLength];
};

static constexpr MacroPair augmentMacroPairs[] = {
    { "BASE_COLOR", "qt_diffuseColor" },
    { "ROUGHNESS", "qt_roughnessAmount" },
    { "METALNESS", "qt_metalnessAmount" },
    { "DIFFUSE_LIGHT", "global_diffuse_light.rgb" },
    { "SPECULAR_LIGHT", "global_specular_light" },
    { "EMISSIVE_LIGHT", "global_emission" },
    { "WORLD_NORMAL", "qt_world_normal" },
    { "WORLD_TANGENT", "qt_tangent" },
    { "WORLD_BINORMAL", "qt_binormal" },
    { "F0", "qt_f0" },
    { "F90", "qt_f90" }
};

constexpr QByteArrayView augmentMacroName(AugmentMacro macro)
{
    return QByteArrayView(augmentMacroPairs[static_cast<size_t>(macro)].macroName);
}

constexpr QByteArrayView augmentMacroReplacement(AugmentMacro macro)
{
    return QByteArrayView(augmentMacroPairs[static_cast<size_t>(macro)].replacement);
}

}

// NOTE: While this class doesn't have graphics resources itself, it manages commands that do!
QSSGRenderUserPass::QSSGRenderUserPass()
    : QSSGRenderGraphObject(Type::RenderPass, FlagT(Flags::HasGraphicsResources))
{

}

QSSGRenderUserPass::~QSSGRenderUserPass()
{
    resetCommands();
}

bool QSSGRenderUserPass::isDirty(DirtyFlag flag) const
{
    return (m_dirtyFlags & FlagT(flag)) != 0;
}

void QSSGRenderUserPass::markDirty(DirtyFlag flag)
{
    m_dirtyFlags |= FlagT(flag);
}

void QSSGRenderUserPass::clearDirty(DirtyFlag flag)
{
    m_dirtyFlags &= ~FlagT(flag);
}

void QSSGRenderUserPass::replaceAugmentMacros(QByteArray &shaderCode, QSSGUserShaderAugmentation &augmentation)
{
    // Tokenize the shader code and replace any identifier that matches one of the augment
    // macros (e.g. BASE_COLOR) with the corresponding built-in variable name. Using the
    // tokenizer means we only ever match whole identifiers - never a substring of a larger
    // identifier - and macros inside comments are skipped, all without re-implementing that
    // logic here.
    QSSGShaderTokenizer tok;
    tok.initialize(shaderCode.constData());

    const char *const begin = shaderCode.constData();
    const char *const end = begin + shaderCode.size();
    const char *lastPos = begin; // Start of the region not yet copied to the result.

    QByteArray result;
    result.reserve(shaderCode.size());

    for (QSSGShaderTokenizer::Token t = tok.next(); t != QSSGShaderTokenizer::Token_EOF; t = tok.next()) {
        if (t != QSSGShaderTokenizer::Token_Identifier)
            continue;

        const QByteArrayView id(tok.identifier, tok.pos - tok.identifier);
        for (size_t i = 0, n = std::size(augmentMacroPairs); i < n; ++i) {
            const AugmentMacro macro = static_cast<AugmentMacro>(i);
            if (id != augmentMacroName(macro))
                continue;

            // Copy everything up to the macro verbatim (preserving comments, whitespace and
            // any non-matching code), then append the replacement in its place.
            const QByteArrayView replacement = augmentMacroReplacement(macro);
            result.append(lastPos, tok.identifier - lastPos);
            result.append(replacement.data(), replacement.size());
            lastPos = tok.pos;
            augmentation.setFeatureFlag(featureForMacro(macro));
            break;
        }
    }

    // No macros were found, leave the shader code untouched.
    if (lastPos == begin)
        return;

    // Copy the trailing remainder after the last replacement.
    result.append(lastPos, end - lastPos);
    shaderCode = std::move(result);
}

void QSSGRenderUserPass::finalizeShaders(const QSSGRenderContextInterface &ctx)
{
    Q_UNUSED(ctx);

    if (materialMode != AugmentMaterial)
        return;

    if (!isDirty(DirtyFlag::ShaderDirty))
        return;

    clearDirty(DirtyFlag::ShaderDirty);

    shaderAugmentation.featureFlags = 0;
    shaderAugmentation.hash.clear();

    const bool hasShaderCode = (shaderAugmentation.hasUserAugmentation());

    if (hasShaderCode) {
        qCDebug(lcRenderUserPass) << "Finalizing shaders for user pass with augment shader code.";

        // Replace any instances of the AugmentMacros with the appropriate built-in variable names
        replaceAugmentMacros(shaderAugmentation.preamble, shaderAugmentation);
        replaceAugmentMacros(shaderAugmentation.body, shaderAugmentation);
        // Used to build the unique shader cache key. The shader key goes in the generated
        // shader as a one line comment so it can't be multiline. Generate hash from the
        // augmented shader preamble and body instead, which gets combined with the material key.
        const QByteArray fullAugmentation = shaderAugmentation.preamble + shaderAugmentation.body;
        shaderAugmentation.hash = QCryptographicHash::hash(QByteArrayView(fullAugmentation),
                                                           QCryptographicHash::Algorithm::Sha1).toHex();

        m_state = State::Ready;
    } else {
        m_state = State::Error;

        qWarning() << "No augment shader code provided for user pass.";
    }

    // Finalize shaders for all commands
    qCDebug(lcRenderUserPass) << "Finalizing shaders for user pass commands";
}

void QSSGRenderUserPass::resetCommands()
{
    qDeleteAll(commands);
    commands.clear();
}

QT_END_NAMESPACE
