/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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


#include "qssgrendernode_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderNode::QSSGRenderNode()
    : QSSGRenderNode(Type::Node)
{

}

QSSGRenderNode::QSSGRenderNode(Type type)
    : QSSGRenderGraphObject(type) {}

QSSGRenderNode::QSSGRenderNode(const QSSGRenderNode &inCloningObject)
    : QSSGRenderGraphObject(inCloningObject)
    , rotation(inCloningObject.rotation) // Radians
    , position(inCloningObject.position)
    , scale(inCloningObject.scale)
    , pivot(inCloningObject.pivot)
    , localOpacity(inCloningObject.localOpacity)
    , localTransform(inCloningObject.localTransform)
    , globalTransform(inCloningObject.globalTransform)
    , globalOpacity(inCloningObject.globalOpacity)
    , skeletonId(inCloningObject.skeletonId)
{
    // for ( SNode* theChild = m_FirstChild; theChild != nullptr; theChild = theChild->m_NextSibling )
    //{
    //	SNode* theClonedChild = static_cast<SNode*>( CGraphObjectFactory::CloneGraphObject(
    //*theChild, inAllocator ) );
    //	AddChild( *theClonedChild );
    //}
}

// Sets this object dirty and walks down the graph setting all
// children who are not dirty to be dirty.
void QSSGRenderNode::markDirty(TransformDirtyFlag inTransformDirty)
{
    if (!flags.testFlag(Flag::TransformDirty))
        flags.setFlag(Flag::TransformDirty, inTransformDirty != TransformDirtyFlag::TransformNotDirty);
    if (!flags.testFlag(Flag::Dirty)) {
        flags.setFlag(Flag::Dirty, true);
        for (QSSGRenderNode *child = firstChild; child; child = child->nextSibling)
            child->markDirty(inTransformDirty);
    }
}

// Calculate global transform and opacity
// Walks up the graph ensure all parents are not dirty so they have
// valid global transforms.

bool QSSGRenderNode::calculateGlobalVariables()
{
    bool retval = flags.testFlag(Flag::Dirty);
    if (retval) {
        flags.setFlag(Flag::Dirty, false);
        if (flags.testFlag(Flag::TransformDirty))
            calculateLocalTransform();
        globalOpacity = localOpacity;
        if (parent) {
            // Layer transforms do not flow down but affect the final layer's rendered
            // representation.
            retval = parent->calculateGlobalVariables() || retval;
            if (parent->type != QSSGRenderGraphObject::Type::Layer) {
                globalOpacity *= parent->globalOpacity;
                if (!flags.testFlag(Flag::IgnoreParentTransform))
                    globalTransform = parent->globalTransform * localTransform;
                else
                    globalTransform = localTransform;
            } else
                globalTransform = localTransform;

            flags.setFlag(Flag::GloballyActive, (flags.testFlag(Flag::Active) && parent->flags.testFlag(Flag::GloballyActive)));
            flags.setFlag(Flag::GloballyPickable, (flags.testFlag(Flag::LocallyPickable) || parent->flags.testFlag(Flag::GloballyPickable)));
        } else {
            globalTransform = localTransform;
            flags.setFlag(Flag::GloballyActive, flags.testFlag(Flag::Active));
            flags.setFlag(Flag::GloballyPickable, flags.testFlag(Flag::LocallyPickable));
        }
    }
    // We always clear dirty in a reasonable manner but if we aren't active
    // there is no reason to tell the universe if we are dirty or not.
    return retval && flags.testFlag(Flag::Active);
}

void QSSGRenderNode::calculateRotationMatrix(QMatrix4x4 &outMatrix) const
{
    outMatrix = QMatrix4x4(rotation.toRotationMatrix());
}

void QSSGRenderNode::calculateLocalTransform()
{
    flags.setFlag(Flag::TransformDirty, false);
    localTransform = QMatrix4x4();
    globalTransform = localTransform;
    float *writePtr = localTransform.data();
    QVector3D theScaledPivot(-pivot[0] * scale[0], -pivot[1] * scale[1], -pivot[2] * scale[2]);
    localTransform(0, 0) = scale[0];
    localTransform(1, 1) = scale[1];
    localTransform(2, 2) = scale[2];

    writePtr[12] = theScaledPivot[0];
    writePtr[13] = theScaledPivot[1];
    writePtr[14] = theScaledPivot[2];

    QMatrix4x4 theRotationTransform;
    calculateRotationMatrix(theRotationTransform);
    // may need column conversion in here somewhere.
    localTransform = theRotationTransform * localTransform;

    writePtr[12] += position[0];
    writePtr[13] += position[1];
    writePtr[14] += position[2];
}

void QSSGRenderNode::setLocalTransformFromMatrix(QMatrix4x4 &inTransform)
{
    flags.setFlag(Flag::TransformDirty);

    // clear pivot
    pivot[0] = pivot[1] = pivot[2] = 0.0f;

    // set translation
    position[0] = inTransform(3, 0);
    position[1] = inTransform(3, 1);
    position[2] = inTransform(3, 2);
    // set scale
    const QVector3D column0(inTransform(0, 0), inTransform(0, 1), inTransform(0, 2));
    const QVector3D column1(inTransform(1, 0), inTransform(1, 1), inTransform(1, 2));
    const QVector3D column2(inTransform(2, 0), inTransform(2, 1), inTransform(2, 2));
    scale[0] = vec3::magnitude(column0);
    scale[1] = vec3::magnitude(column1);
    scale[2] = vec3::magnitude(column2);
    // make sure there is no zero value
    scale[0] = (scale[0] == 0.0f) ? 1.0f : scale[0];
    scale[1] = (scale[1] == 0.0f) ? 1.0f : scale[1];
    scale[2] = (scale[2] == 0.0f) ? 1.0f : scale[2];

    // extract rotation by first dividing through scale value
    float invScaleX = 1.0f / scale[0];
    float invScaleY = 1.0f / scale[1];
    float invScaleZ = 1.0f / scale[2];

    inTransform(0, 0) *= invScaleX;
    inTransform(0, 1) *= invScaleX;
    inTransform(0, 2) *= invScaleX;
    inTransform(1, 0) *= invScaleY;
    inTransform(1, 1) *= invScaleY;
    inTransform(1, 2) *= invScaleY;
    inTransform(2, 0) *= invScaleZ;
    inTransform(2, 1) *= invScaleZ;
    inTransform(2, 2) *= invScaleZ;

    float rotationMatrixData[9] = { inTransform(0, 0), inTransform(0, 1), inTransform(0, 2),
                                    inTransform(1, 0), inTransform(1, 1), inTransform(1, 2),
                                    inTransform(2, 0), inTransform(2, 1), inTransform(2, 2) };

    QMatrix3x3 theRotationMatrix(rotationMatrixData);
    rotation = QQuaternion::fromRotationMatrix(theRotationMatrix).normalized();
}

void QSSGRenderNode::addChild(QSSGRenderNode &inChild)
{
    // Adding children to a layer does not reset parent
    // because layers can share children over with other layers
    if (this->type != QSSGRenderNode::Type::Layer) {
        if (inChild.parent)
            inChild.parent->removeChild(inChild);
        inChild.parent = this;
    }
    if (firstChild == nullptr) {
        firstChild = &inChild;
        inChild.nextSibling = nullptr;
        inChild.previousSibling = nullptr;
    } else {
        QSSGRenderNode *lastChild = getLastChild();
        if (lastChild) {
            lastChild->nextSibling = &inChild;
            inChild.previousSibling = lastChild;
            inChild.nextSibling = nullptr;
        } else {
            Q_ASSERT(false); // no last child but first child isn't null?
        }
    }
}

void QSSGRenderNode::addChildrenToLayer(QSSGRenderNode &inChildren)
{
    // Adding children to a layer does not reset parent
    // because layers can share children over with other layers
    if (firstChild == nullptr) {
        firstChild = &inChildren;
        inChildren.previousSibling = nullptr;
    } else {
        QSSGRenderNode *lastChild = getLastChild();
        if (lastChild) {
            lastChild->nextSibling = &inChildren;
            inChildren.previousSibling = lastChild;
        } else {
            Q_ASSERT(false); // no last child but first child isn't null?
        }
    }
}


void QSSGRenderNode::removeChild(QSSGRenderNode &inChild)
{
    // Removing children from a layer does not care about parenting
    // because layers can share children over with other layers
    if (this->type != QSSGRenderNode::Type::Layer) {
        if (inChild.parent != this) {
            Q_ASSERT(false);
            return;
        }
    }

    for (QSSGRenderNode *child = firstChild; child; child = child->nextSibling) {
        if (child == &inChild) {
            if (child->previousSibling)
                child->previousSibling->nextSibling = child->nextSibling;
            if (child->nextSibling)
                child->nextSibling->previousSibling = child->previousSibling;
            child->parent = nullptr;
            if (firstChild == child)
                firstChild = child->nextSibling;
            child->nextSibling = nullptr;
            child->previousSibling = nullptr;
            return;
        }
    }
    Q_ASSERT(false);
}

QSSGRenderNode *QSSGRenderNode::getLastChild()
{
    QSSGRenderNode *lastChild = nullptr;
    // empty loop intentional
    for (lastChild = firstChild; lastChild && lastChild->nextSibling; lastChild = lastChild->nextSibling) {
    }
    return lastChild;
}

void QSSGRenderNode::removeFromGraph()
{
    if (parent)
        parent->removeChild(*this);

    nextSibling = nullptr;

    // Orphan all of my children.
    QSSGRenderNode *nextSibling = nullptr;
    for (QSSGRenderNode *child = firstChild; child != nullptr; child = nextSibling) {
        child->previousSibling = nullptr;
        child->parent = nullptr;
        nextSibling = child->nextSibling;
        child->nextSibling = nullptr;
    }
}

QSSGBounds3 QSSGRenderNode::getBounds(const QSSGRef<QSSGBufferManager> &inManager,
                                         bool inIncludeChildren,
                                         QSSGRenderNodeFilterInterface *inChildFilter) const
{
    QSSGBounds3 retval = QSSGBounds3::empty();
    if (inIncludeChildren)
        retval = getChildBounds(inManager, inChildFilter);

    if (type == QSSGRenderGraphObject::Type::Model)
        retval.include(static_cast<const QSSGRenderModel *>(this)->getModelBounds(inManager));
    return retval;
}

QSSGBounds3 QSSGRenderNode::getChildBounds(const QSSGRef<QSSGBufferManager> &inManager,
                                              QSSGRenderNodeFilterInterface *inChildFilter) const
{
    QSSGBounds3 retval = QSSGBounds3::empty();
    for (QSSGRenderNode *child = firstChild; child != nullptr; child = child->nextSibling) {
        if (inChildFilter == nullptr || inChildFilter->includeNode(*child)) {
            QSSGBounds3 childBounds;
            if (child->flags.testFlag(Flag::TransformDirty))
                child->calculateLocalTransform();
            childBounds = child->getBounds(inManager);
            if (!childBounds.isEmpty()) {
                // Transform the bounds into our local space.
                childBounds.transform(child->localTransform);
                retval.include(childBounds);
            }
        }
    }
    return retval;
}

QVector3D QSSGRenderNode::getGlobalPos() const
{
    return QVector3D(globalTransform(0, 3), globalTransform(1, 3), globalTransform(2, 3));
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
    // ### This code has been checked to be corect
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(globalTransform);
    theDirMatrix = mat33::getInverse(theDirMatrix).transposed();
    QVector3D theOriginalDir(0, 0, -1);
    QVector3D retval = mat33::transform(theDirMatrix, theOriginalDir);
    // Should already be normalized, but whatever
    retval.normalize();
    return retval;
}

QVector3D QSSGRenderNode::getGlobalPivot() const
{
    QVector3D retval(position);
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
    QMatrix3x3 outNormalMatrix = mat44::getUpper3x3(globalTransform);
    return mat33::getInverse(outNormalMatrix).transposed();
}

QT_END_NAMESPACE
