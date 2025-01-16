// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderLayer::QSSGRenderLayer()
    : QSSGRenderNode(QSSGRenderNode::Type::Layer)
    , firstEffect(nullptr)
    , antialiasingMode(QSSGRenderLayer::AAMode::NoAA)
    , antialiasingQuality(QSSGRenderLayer::AAQuality::High)
    , background(QSSGRenderLayer::Background::Transparent)
    , temporalAAStrength(0.3f)
    , ssaaMultiplier(1.5f)
    , specularAAEnabled(false)
    , oitMethod(OITMethod::None)
    , oitMethodDirty(false)
    , tonemapMode(TonemapMode::Linear)
{
    flags = { FlagT(LocalState::Active) | FlagT(GlobalState::Active) }; // The layer node is alway active and not dirty.
}

QSSGRenderLayer::~QSSGRenderLayer()
{
    delete importSceneNode;
    importSceneNode = nullptr;
    delete renderData;
}

void QSSGRenderLayer::setProbeOrientation(const QVector3D &angles)
{
    if (angles != lightProbeSettings.probeOrientationAngles) {
        lightProbeSettings.probeOrientationAngles = angles;
        lightProbeSettings.probeOrientation = QQuaternion::fromEulerAngles(lightProbeSettings.probeOrientationAngles).toRotationMatrix();
    }
}

void QSSGRenderLayer::addEffect(QSSGRenderEffect &inEffect)
{
    // Effects need to be rendered in reverse order as described in the file.
    inEffect.m_nextEffect = firstEffect;
    firstEffect = &inEffect;
}

bool QSSGRenderLayer::hasEffect(QSSGRenderEffect *inEffect) const
{
    for (auto currentEffect = firstEffect; currentEffect != nullptr; currentEffect = currentEffect->m_nextEffect) {
        if (currentEffect == inEffect)
            return true;
    }
    return false;
}

void QSSGRenderLayer::setImportScene(QSSGRenderNode &rootNode)
{
    // We create a dummy node to represent the imported scene tree, as we
    // do absolutely not want to change the node links in that tree!
    if (importSceneNode == nullptr) {
        importSceneNode = new QSSGRenderNode(QSSGRenderGraphObject::Type::ImportScene);
        // Now we can add the dummy node to the layers child list
        children.push_back(*importSceneNode);
    } else {
        importSceneNode->children.clear(); // Clear the list (or the list will modify the rootNode)
    }

    // The imported scene root node is now a child of the dummy node
    auto &importChildren = importSceneNode->children;
    Q_ASSERT(importChildren.isEmpty());
    // We don't want the list to modify our node, so we set the tail and head manually.
    importChildren.m_head = importChildren.m_tail = &rootNode;
}

void QSSGRenderLayer::removeImportScene(QSSGRenderNode &rootNode)
{
    if (importSceneNode && !importSceneNode->children.isEmpty()) {
        if (&importSceneNode->children.back() == &rootNode)
            importSceneNode->children.clear();
    }
}

QT_END_NAMESPACE
