// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include "qssgrendernode_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>

#include <QtQuick3DUtils/private/qssgplane_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderNode::QSSGRenderNode()
    : QSSGRenderNode(Type::Node)
{
}

QSSGRenderNode::QSSGRenderNode(Type type)
    : QSSGRenderGraphObject(type)
{
    globalTransform = localTransform = calculateTransformMatrix({}, initScale, {}, {});
}

QSSGRenderNode::~QSSGRenderNode()
    = default;

void QSSGRenderNode::markDirty(DirtyFlag dirtyFlag)
{
    if ((flags & FlagT(dirtyFlag)) == 0) { // If not already marked
        flags |= FlagT(dirtyFlag);
        const bool markSubtreeDirty = ((FlagT(dirtyFlag) & FlagT(DirtyFlag::GlobalValuesDirty)) != 0);
        if (markSubtreeDirty) {
            for (auto &cld : children)
                cld.markDirty(dirtyFlag);
        }
    }
}

void QSSGRenderNode::clearDirty(DirtyFlag dirtyFlag)
{
    flags &= ~FlagT(dirtyFlag);
}

void QSSGRenderNode::setState(LocalState state, bool on)
{
    const bool changed = (getLocalState(state) != on);
    if (changed) { // Mark state dirty
        flags = on ? (flags | FlagT(state)) : (flags & ~FlagT(state));

        // Mark state dirty
        switch (state) {
        case QSSGRenderNode::LocalState::Active:
            markDirty(DirtyFlag::ActiveDirty);
            break;
        case QSSGRenderNode::LocalState::Pickable:
            markDirty(DirtyFlag::PickableDirty);
            break;
        }
    }

}

// Calculate global transform and opacity
// Walks up the graph ensure all parents are not dirty so they have
// valid global transforms.

bool QSSGRenderNode::calculateGlobalVariables()
{
    bool retval = isDirty(DirtyFlag::GlobalValuesDirty);
    if (retval) {
        globalOpacity = localOpacity;
        globalTransform = localTransform;

        if (parent) {
            retval = parent->calculateGlobalVariables() || retval;
            const bool globallyActive = getLocalState(LocalState::Active) && parent->getGlobalState(GlobalState::Active);
            flags = globallyActive ? (flags | FlagT(GlobalState::Active)) : (flags & ~FlagT(GlobalState::Active));
            const bool globallyPickable = getLocalState(LocalState::Pickable) || parent->getGlobalState(GlobalState::Pickable);
            flags = globallyPickable ? (flags | FlagT(GlobalState::Pickable)) : (flags & ~FlagT(GlobalState::Pickable));
            globalOpacity *= parent->globalOpacity;
            // Skip calculating the transform for non-active nodes
            if (globallyActive && parent->type != QSSGRenderGraphObject::Type::Layer) {
                globalTransform = parent->globalTransform * localTransform;
                if (this == instanceRoot) {
                    globalInstanceTransform = parent->globalTransform;
                    localInstanceTransform = localTransform;
                } else if (instanceRoot) {
                    globalInstanceTransform = instanceRoot->globalInstanceTransform;
                    //### technically O(n^2) -- we could cache localInstanceTransform if every node in the
                    // tree is guaranteed to have the same instance root. That would require an API change.
                    localInstanceTransform = localTransform;
                    auto *p = parent;
                    while (p) {
                        if (p == instanceRoot) {
                            localInstanceTransform = p->localInstanceTransform * localInstanceTransform;
                            break;
                        }
                        localInstanceTransform = p->localTransform * localInstanceTransform;
                        p = p->parent;
                    }
                } else {
                    // By default, we do magic: translation is applied to the global instance transform,
                    // while scale/rotation is local

                    localInstanceTransform = localTransform;
                    auto &localInstanceMatrix =  *reinterpret_cast<float (*)[4][4]>(localInstanceTransform.data());
                    QVector3D localPos{localInstanceMatrix[3][0], localInstanceMatrix[3][1], localInstanceMatrix[3][2]};
                    localInstanceMatrix[3][0] = 0;
                    localInstanceMatrix[3][1] = 0;
                    localInstanceMatrix[3][2] = 0;
                    globalInstanceTransform = parent->globalTransform;
                    globalInstanceTransform.translate(localPos);
                }
            }
        } else {
            const bool globallyActive = getLocalState(LocalState::Active);
            flags = globallyActive ? (flags | FlagT(GlobalState::Active)) : (flags & ~FlagT(GlobalState::Active));
            const bool globallyPickable = getLocalState(LocalState::Pickable);
            flags = globallyPickable ? (flags | FlagT(GlobalState::Pickable)) : (flags & ~FlagT(GlobalState::Pickable));
            localInstanceTransform = localTransform;
            globalInstanceTransform = {};
        }
        // Clear dirty flags
        clearDirty(DirtyFlag::GlobalValuesDirty);
    }
    // We always clear dirty in a reasonable manner but if we aren't active
    // there is no reason to tell the universe if we are dirty or not.
    return retval && getLocalState(LocalState::Active);
}

QMatrix4x4 QSSGRenderNode::calculateTransformMatrix(QVector3D position, QVector3D scale, QVector3D pivot, QQuaternion rotation)
{
    QMatrix4x4 transform;

    // Offset the origin (this is our pivot point)
    auto offset = (-pivot * scale);

    // Scale
    transform(0, 0) = scale[0];
    transform(1, 1) = scale[1];
    transform(2, 2) = scale[2];

    // Offset (before rotation)
    transform(0, 3) = offset[0];
    transform(1, 3) = offset[1];
    transform(2, 3) = offset[2];

    // rotate
    transform = QMatrix4x4{rotation.toRotationMatrix()} * transform;

    // translate
    transform(0, 3) += position[0];
    transform(1, 3) += position[1];
    transform(2, 3) += position[2];

    return transform;
}

void QSSGRenderNode::addChild(QSSGRenderNode &inChild)
{
    // Adding children to a layer does not reset parent
    // because layers can share children over with other layers
    if (type != QSSGRenderNode::Type::Layer) {
        if (inChild.parent && inChild.parent != this)
            inChild.parent->removeChild(inChild);
        inChild.parent = this;
    }
    children.push_back(inChild);
    inChild.markDirty(DirtyFlag::GlobalValuesDirty);
}

void QSSGRenderNode::removeChild(QSSGRenderNode &inChild)
{
    if (Q_UNLIKELY(type != QSSGRenderNode::Type::Layer && inChild.parent != this)) {
        Q_ASSERT(inChild.parent == this);
        return;
    }

    inChild.parent = nullptr;
    children.remove(inChild);
    inChild.markDirty(DirtyFlag::GlobalValuesDirty);
}

void QSSGRenderNode::removeFromGraph()
{
    if (parent)
        parent->removeChild(*this);

    // Orphan all of my children.
    for (auto it = children.begin(), end = children.end(); it != end;) {
        auto &removedChild = *it++;
        children.remove(removedChild);
        removedChild.parent = nullptr;
    }
}

QSSGBounds3 QSSGRenderNode::getBounds(QSSGBufferManager &inManager,
                                      bool inIncludeChildren) const
{
    QSSGBounds3 retval;
    if (inIncludeChildren)
        retval = getChildBounds(inManager);

    if (type == QSSGRenderGraphObject::Type::Model) {
        auto model = static_cast<const QSSGRenderModel *>(this);
        retval.include(inManager.getModelBounds(model));
    }
    return retval;
}

QSSGBounds3 QSSGRenderNode::getChildBounds(QSSGBufferManager &inManager) const
{
    QSSGBounds3 retval;
    QSSGBounds3 childBounds;
    for (auto &child : children) {
        childBounds = child.getBounds(inManager);
        if (!childBounds.isEmpty()) {
            // Transform the bounds into our local space.
            childBounds.transform(child.localTransform);
            retval.include(childBounds);
        }
    }
    return retval;
}

QVector3D QSSGRenderNode::getDirection() const
{
    const float *dataPtr(globalTransform.data());
    QVector3D retval(dataPtr[8], dataPtr[9], dataPtr[10]);
    retval.normalize();
    return retval;
}

QVector3D QSSGRenderNode::getScalingCorrectDirection() const
{
    QMatrix3x3 theDirMatrix = globalTransform.normalMatrix();
    QVector3D theOriginalDir(0, 0, -1);
    QVector3D retval = QSSGUtils::mat33::transform(theDirMatrix, theOriginalDir);
    // Should already be normalized, but whatever
    retval.normalize();
    return retval;
}

QVector3D QSSGRenderNode::getGlobalPivot() const
{
    QVector3D retval(QSSGUtils::mat44::getPosition(localTransform));
    retval.setZ(retval.z() * -1);

    if (parent && parent->type != QSSGRenderGraphObject::Type::Layer) {
        const QVector4D direction(retval.x(), retval.y(), retval.z(), 1.0f);
        const QVector4D result = parent->globalTransform * direction;
        return QVector3D(result.x(), result.y(), result.z());
    }

    return retval;
}

void QSSGRenderNode::calculateMVPAndNormalMatrix(const QMatrix4x4 &inViewProjection, QMatrix4x4 &outMVP, QMatrix3x3 &outNormalMatrix) const
{
    outMVP = inViewProjection * globalTransform;
    outNormalMatrix = calculateNormalMatrix();
}

QMatrix3x3 QSSGRenderNode::calculateNormalMatrix() const
{
    // NB! QMatrix4x4:normalMatrix() uses double precision for determinant
    // calculations when inverting the matrix, which is good and is important
    // in practice e.g. in scenes with with small scale factors.

    return globalTransform.normalMatrix();
}

QT_END_NAMESPACE
