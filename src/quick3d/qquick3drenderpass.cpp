// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3drenderpass_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderuserpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

QT_BEGIN_NAMESPACE

QQuick3DRenderPass::QQuick3DRenderPass(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::RenderPass)), parent)
    , QQuick3DPropertyChangedTracker(this, QQuick3DSuperClassInfo<QQuick3DRenderPass>())
{
}

QSSGRenderGraphObject *QQuick3DRenderPass::updateSpatialNode(QSSGRenderGraphObject *node)
{
    QSSGRenderUserPass *renderPassNode = static_cast<QSSGRenderUserPass *>(node);

    bool newBackendNode = false;
    if (!renderPassNode) {
        renderPassNode = new QSSGRenderUserPass;
        newBackendNode = true;
    }

    const bool fullUpdate = newBackendNode  || (m_dirtyAttributes & Dirty::TextureDirty);

    auto &shaderAugmentation = renderPassNode->shaderAugmentation;
    auto &uniformProps = shaderAugmentation.propertyUniforms;

    if (fullUpdate) {
        markAllDirty();

        // Properties -> uniforms.
        // NOTE: Calling extractProperties clears existing properties
        extractProperties(uniformProps);

        // Commands
        renderPassNode->resetCommands();
        for (QQuick3DShaderUtilsRenderCommand *command : std::as_const(m_commands)) {
            if (auto *cmd = command->cloneCommand())
                renderPassNode->commands.push_back(cmd);
        }
    }

    // Update the property values
    if (m_dirtyAttributes & Dirty::PropertyDirty) {
        for (const auto &prop : std::as_const(uniformProps)) {
            auto p = metaObject()->property(prop.pid);
            if (Q_LIKELY(p.isValid())) {
                QVariant v = p.read(this);
                if (v.isValid()) {
                    if (v.metaType().id() == qMetaTypeId<QQuick3DTexture *>()) {
                        QQuick3DTexture *tex = v.value<QQuick3DTexture *>();
                        auto *po = QQuick3DObjectPrivate::get(tex);
                        QSSGRenderImage *ri = static_cast<QSSGRenderImage *>(po->spatialNode);
                        prop.value = QVariant::fromValue(ri);
                    } else {
                        prop.value = v;
                    }
                }
            }
        }
    }

    // Clear Dirty
    if (m_dirtyAttributes & Dirty::ClearDirty) {
        renderPassNode->clearBuffers = m_enableClearBuffers;
        renderPassNode->clearColor = m_clearColor;
        renderPassNode->depthStencilClearValue = { m_depthClearValue, m_stencilClearValue };
    }

    if (m_dirtyAttributes & Dirty::PassTypeDirty) {
        switch (m_passMode) {
        case UserPass:
            renderPassNode->passMode = QSSGRenderUserPass::UserPass;
            break;
        case SkyboxPass:
            renderPassNode->passMode = QSSGRenderUserPass::SkyboxPass;
            break;
        case Item2DPass:
            renderPassNode->passMode = QSSGRenderUserPass::Item2DPass;
            break;
        }
    }

    m_dirtyAttributes = 0;

    // If not a user pass, we're done
    if (m_passMode != UserPass)
        return renderPassNode;

    renderPassNode->materialMode = QSSGRenderUserPass::MaterialModes(m_materialMode);
    if (renderPassNode->materialMode == QSSGRenderUserPass::OverrideMaterial) {
        if (m_overrideMaterial) {
            // Set the backend material
            QSSGRenderGraphObject *graphObject = QQuick3DObjectPrivate::get(m_overrideMaterial)->spatialNode;
            if (graphObject)
                renderPassNode->overrideMaterial = graphObject;
            else
                markDirty(OverrideMaterialDirty); // Try again next time
        } else {
            // Set nullptr
            renderPassNode->overrideMaterial = nullptr;
        }
    } else if (renderPassNode->materialMode == QSSGRenderUserPass::OriginalMaterial) {
        // Nothing to do
    } else if (renderPassNode->materialMode == QSSGRenderUserPass::AugmentMaterial) {
        // Augment Shaders
        if (!m_augmentShader.isEmpty()) {
            const QQmlContext *context = qmlContext(this);
            QByteArray shaderPathKey("augment material --");
            QByteArray augment = QSSGShaderUtils::resolveShader(m_augmentShader, context, shaderPathKey);
            QByteArray augmentSnippet;
            QByteArray augmentPreamble;

            // We have to pick apart the shader string such that the contents of the:
            // void MAIN_FRAGMENT_AUGMENT() { }
            // function are taken out, and will get added to the end of the shader generation
            // and the goal is to overwrite the "output" of the shader

            // We also need to scan the who shader code for certain "keywords" so that we know
            // what features to enable in the original material.

            // Everything else outsode of MAIN_FRAGMENT_AUGMENT function ends up being preamble code
            // that will get pasted in before the real main().  So that will include helper functions and
            // resolvable #includes etc.

            static const char *mainFuncStart = "void MAIN_FRAGMENT_AUGMENT()";
            qsizetype mainFuncIdx = augment.indexOf(mainFuncStart);
            if (mainFuncIdx != -1) {
                qsizetype braceOpenIdx = augment.indexOf('{', mainFuncIdx + int(strlen(mainFuncStart)));
                if (braceOpenIdx != -1) {
                    qsizetype braceCloseIdx = braceOpenIdx;
                    qsizetype openBraces = 1;
                    while (openBraces > 0 && braceCloseIdx + 1 < augment.size()) {
                        braceCloseIdx++;
                        if (augment[braceCloseIdx] == '{')
                            openBraces++;
                        else if (augment[braceCloseIdx] == '}')
                            openBraces--;
                    }
                    if (openBraces == 0) {
                        // We found the closing brace
                        augmentSnippet = augment.mid(braceOpenIdx + 1, braceCloseIdx - braceOpenIdx - 1);
                        augmentPreamble = augment.left(mainFuncIdx);
                        augmentPreamble += augment.mid(braceCloseIdx + 1);
                    } else {
                        qWarning("QQuick3DRenderPass: Could not find the closing brace of MAIN_FRAGMENT_AUGMENT() in shader %s", qPrintable(m_augmentShader.toString()));
                    }
                } else {
                    qWarning("QQuick3DRenderPass: Could not find the opening brace of MAIN_FRAGMENT_AUGMENT() in shader %s", qPrintable(m_augmentShader.toString()));
                }
            } else {
                qWarning("QQuick3DRenderPass: Could not find MAIN_FRAGMENT_AUGMENT() function in shader %s", qPrintable(m_augmentShader.toString()));
            }

            renderPassNode->shaderAugmentation.body = augmentSnippet;
            renderPassNode->shaderAugmentation.preamble = augmentPreamble;
            renderPassNode->markDirty(QSSGRenderUserPass::DirtyFlag::ShaderDirty);
        }
    }

    return renderPassNode;
}

void QQuick3DRenderPass::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DRenderPass::markTrackedPropertyDirty(QMetaProperty property, DirtyPropertyHint hint)
{
    Q_UNUSED(property);

    // FIXME: As with the property tracking for Effects and Custom materials we
    // should really track which property changed and only update that one.
    if (hint == DirtyPropertyHint::Reference) {
        // FIXME: We should verify that the property is actually a texture property.
        markDirty(Dirty::TextureDirty);
    } else {
        markDirty(Dirty::PropertyDirty);
    }
}

void QQuick3DRenderPass::onMaterialDestroyed(QObject *object)
{
    if (m_overrideMaterial == object) {
        m_overrideMaterial = nullptr;
        emit overrideMaterialChanged();
        markDirty(OverrideMaterialDirty);
    }
}

void QQuick3DRenderPass::qmlAppendCommand(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, QQuick3DShaderUtilsRenderCommand *command)
{
    if (!command)
        return;

    QQuick3DRenderPass *that = qobject_cast<QQuick3DRenderPass *>(list->object);
    that->m_commands.push_back(command);
    that->markDirty(CommandsDirty);
}

QQuick3DShaderUtilsRenderCommand *QQuick3DRenderPass::qmlCommandAt(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, qsizetype index)
{
    QQuick3DRenderPass *that = qobject_cast<QQuick3DRenderPass *>(list->object);
    return that->m_commands.at(index);
}

qsizetype QQuick3DRenderPass::qmlCommandCount(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DRenderPass *that = qobject_cast<QQuick3DRenderPass *>(list->object);
    return that->m_commands.size();
}

void QQuick3DRenderPass::qmlCommandClear(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DRenderPass *that = qobject_cast<QQuick3DRenderPass *>(list->object);
    that->m_commands.clear();
    that->markDirty(CommandsDirty);
}

void QQuick3DRenderPass::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    Q_UNUSED(sceneManager)
}

void QQuick3DRenderPass::markDirty(Dirty type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

QQmlListProperty<QQuick3DShaderUtilsRenderCommand> QQuick3DRenderPass::commands()
{
    return QQmlListProperty<QQuick3DShaderUtilsRenderCommand>(this,
                                                              nullptr,
                                                              QQuick3DRenderPass::qmlAppendCommand,
                                                              QQuick3DRenderPass::qmlCommandCount,
                                                              QQuick3DRenderPass::qmlCommandAt,
                                                              QQuick3DRenderPass::qmlCommandClear);
}

QColor QQuick3DRenderPass::clearColor() const
{
    return m_clearColor;
}

void QQuick3DRenderPass::setClearColor(const QColor &newClearColor)
{
    if (m_clearColor == newClearColor)
        return;
    m_clearColor = newClearColor;
    emit clearColorChanged();
    markDirty(ClearDirty);
}

QQuick3DRenderPass::MaterialModes QQuick3DRenderPass::materialMode() const
{
    return m_materialMode;
}

void QQuick3DRenderPass::setMaterialMode(MaterialModes newMaterialMode)
{
    if (m_materialMode == newMaterialMode)
        return;
    m_materialMode = newMaterialMode;
    emit materialModeChanged();
    markDirty(MaterialModeDirty);
}

QQuick3DMaterial *QQuick3DRenderPass::overrideMaterial() const
{
    return m_overrideMaterial;
}

void QQuick3DRenderPass::setOverrideMaterial(QQuick3DMaterial *newOverrideMaterial)
{
    if (m_overrideMaterial == newOverrideMaterial)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DRenderPass::setOverrideMaterial, newOverrideMaterial, m_overrideMaterial);

    m_overrideMaterial = newOverrideMaterial;
    emit overrideMaterialChanged();
    markDirty(OverrideMaterialDirty);
}

QUrl QQuick3DRenderPass::augmentShader() const
{
    return m_augmentShader;
}

void QQuick3DRenderPass::setAugmentShader(const QUrl &newAugmentShader)
{
    if (m_augmentShader == newAugmentShader)
        return;
    m_augmentShader = newAugmentShader;
    emit augmentShaderChanged();
    markDirty(AugmentShaderDirty);
}

QQuick3DRenderPass::PassMode QQuick3DRenderPass::passMode() const
{
    return m_passMode;
}

void QQuick3DRenderPass::setPassMode(PassMode newPassMode)
{
    if (m_passMode == newPassMode)
        return;
    m_passMode = newPassMode;
    emit passModeChanged();
    markDirty(PassTypeDirty);
}

float QQuick3DRenderPass::depthClearValue() const
{
    return m_depthClearValue;
}

void QQuick3DRenderPass::setDepthClearValue(float newDepthClearValue)
{
    if (qFuzzyCompare(m_depthClearValue, newDepthClearValue))
        return;
    m_depthClearValue = newDepthClearValue;
    emit depthClearValueChanged();
    markDirty(ClearDirty);
}

quint32 QQuick3DRenderPass::stencilClearValue() const
{
    return m_stencilClearValue;
}

void QQuick3DRenderPass::setStencilClearValue(quint32 newStencilClearValue)
{
    if (m_stencilClearValue == newStencilClearValue)
        return;
    m_stencilClearValue = newStencilClearValue;
    emit stencilClearValueChanged();
    markDirty(ClearDirty);
}

bool QQuick3DRenderPass::enableClearBuffers() const
{
    return m_enableClearBuffers;
}

void QQuick3DRenderPass::setEnableClearBuffers(bool newEnableClearBuffers)
{
    if (m_enableClearBuffers == newEnableClearBuffers)
        return;
    m_enableClearBuffers = newEnableClearBuffers;
    emit enableClearBuffersChanged();
    markDirty(ClearDirty);
}

QT_END_NAMESPACE
