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

#ifndef QSSG_RENDERER_H
#define QSSG_RENDERER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobjectpickquery_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderray_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>

#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE

typedef void *QSSGRenderInstanceId;

struct QSSGRenderLayer;
class QSSGRendererImpl;
class QSSGRenderContextInterface;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRendererInterface
{
public:
    QAtomicInt ref;
    virtual ~QSSGRendererInterface() {}
    virtual void enableLayerGpuProfiling(bool inEnabled) = 0;
    virtual bool isLayerGpuProfilingEnabled() const = 0;

    // Called when you have changed the number or order of children of a given node.
    virtual void childrenUpdated(QSSGRenderNode &inParent) = 0;

    // The QSSGRenderContextInterface calls these, clients should not.
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // RHI only
    virtual QSSGRhiQuadRenderer *rhiQuadRenderer() = 0;

    // Returns true if this layer or a sibling was dirty.
    virtual bool prepareLayerForRender(QSSGRenderLayer &inLayer, const QSize &surfaceSize) = 0;

    // RHI-only
    virtual void rhiPrepare(QSSGRenderLayer &inLayer) = 0;
    virtual void rhiRender(QSSGRenderLayer &inLayer) = 0;


    // Studio option to disable picking against sub renderers.  This allows better interaction
    // in studio.
    // In pick siblings measn pick the layer siblings; this is the normal behavior.
    // InPickEverything means ignore the node's pick flags; this allows us to only pick things
    // that have handlers
    // in some cases and just pick everything in other things.
    virtual void pickRenderPlugins(bool inPick) = 0;
    virtual QSSGRenderPickResult pick(QSSGRenderLayer &inLayer,
                                        const QVector2D &inViewportDimensions,
                                        const QVector2D &inMouseCoords,
                                        bool inPickSiblings = true,
                                        bool inPickEverything = false) = 0;
    virtual QSSGRenderPickResult syncPick(const QSSGRenderLayer &inLayer,
                                          const QSSGRef<QSSGBufferManager> &bufferManager,
                                          const QVector2D &inViewportDimensions,
                                          const QVector2D &inMouseCoords) = 0;

    // Return the relative hit position, in UV space, of a mouse pick against this object.
    // We need the node in order to figure out which layer rendered this object.
    // We need mapper objects if this is a in a subpresentation because we have to know how
    // to map the mouse coordinates into the subpresentation.  So for instance if inNode is in
    // a subpres then we need to know which image is displaying the subpres in order to map
    // the mouse coordinates into the subpres's render space.
    virtual QSSGOption<QVector2D> facePosition(QSSGRenderNode &inNode,
                                                 QSSGBounds3 inBounds,
                                                 const QMatrix4x4 &inGlobalTransform,
                                                 const QVector2D &inViewportDimensions,
                                                 const QVector2D &inMouseCoords,
                                                 QSSGDataView<QSSGRenderGraphObject *> inMapperObjects,
                                                 QSSGRenderBasisPlanes inIsectPlane) = 0;

    virtual QVector3D unprojectToPosition(QSSGRenderNode &inNode, QVector3D &inPosition, const QVector2D &inMouseVec) const = 0;
    virtual QVector3D unprojectWithDepth(QSSGRenderNode &inNode, QVector3D &inPosition, const QVector3D &inMouseVec) const = 0;
    virtual QVector3D projectPosition(QSSGRenderNode &inNode, const QVector3D &inPosition) const = 0;

    // Get the mouse coordinates as they relate to a given layer
    virtual QSSGOption<QVector2D> getLayerMouseCoords(QSSGRenderLayer &inLayer,
                                                        const QVector2D &inMouseCoords,
                                                        const QVector2D &inViewportDimensions,
                                                        bool forceImageIntersect = false) const = 0;

    // Returns true if the renderer expects new frame to be rendered
    // Happens when progressive AA is enabled
    virtual bool rendererRequestsFrames() const = 0;

    static QSSGRef<QSSGRendererInterface> createRenderer(QSSGRenderContextInterface *inContext);
};
QT_END_NAMESPACE

#endif
