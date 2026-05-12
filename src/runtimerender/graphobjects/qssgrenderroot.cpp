// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qssgrenderroot_p.h"
#include "qssgrenderlayer_p.h"

#include "../rendererimpl/qssgrenderdata_p.h"

QT_BEGIN_NAMESPACE

QSSGRenderRoot::QSSGRenderRoot()
    : QSSGRenderNode(Type::Root)
    , m_gnd(std::make_shared<QSSGGlobalRenderNodeData>(this))
{
    rootNodeRef = &self;
    localTransform = calculateTransformMatrix({}, initScale, {}, {});
}

QSSGRenderRoot::~QSSGRenderRoot()
{
    if (m_gnd)
        m_gnd->invalidate();
}

void QSSGRenderRoot::markDirty(DirtyFlag dirtyFlag)
{
    m_rootDirtyFlags |= FlagT(dirtyFlag);
    QSSGRenderNode::markDirty(QSSGRenderNode::DirtyFlag::SubNodeDirty);
}

void QSSGRenderRoot::clearDirty(DirtyFlag dirtyFlag)
{
    m_rootDirtyFlags &= ~FlagT(dirtyFlag);
    QSSGRenderNode::clearDirty(QSSGRenderNode::DirtyFlag::SubNodeDirty);
}

void QSSGRenderRoot::reindex()
{
    m_gnd->reindex();
    for (QSSGRenderNode &chld : children) {
        if (QSSG_GUARD_X(chld.type == QSSGRenderNode::Type::Layer, "Layer type mismatch"))
            static_cast<QSSGRenderLayer &>(chld).markDirty(QSSGRenderLayer::DirtyFlag::TreeDirty);
    }
    clearDirty(QSSGRenderRoot::DirtyFlag::TreeDirty);
}

QT_END_NAMESPACE
