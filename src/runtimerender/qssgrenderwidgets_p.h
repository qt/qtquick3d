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

#ifndef QSSG_RENDER_WIDGETS_H
#define QSSG_RENDER_WIDGETS_H

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

#include <QtCore/qpair.h>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>
#include <QtGui/QVector3D>

#include <QtQuick3DUtils/private/qssgoption_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtQuick3DRender/private/qssgrendervertexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderindexbuffer_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContext;

struct QSSGWidgetRenderInformation
{
    // Just the rotation component of the nodeparenttocamera.
    QMatrix3x3 m_normalMatrix;
    // The node parent's global transform multiplied by the inverse camera global transfrom;
    // basically the MV from model-view-projection
    QMatrix4x4 m_nodeParentToCamera;
    // Projection that accounts for layer scaling
    QMatrix4x4 m_layerProjection;
    // Pure camera projection without layer scaling
    QMatrix4x4 m_pureProjection;
    // A look at matrix that will rotate objects facing directly up
    // the Z axis such that the point to the camera.
    QMatrix3x3 m_lookAtMatrix;
    // Conversion from world to camera position so world points not in object
    // local space can be converted to camera space without going through the node's
    // inverse global transform
    QMatrix4x4 m_cameraGlobalInverse;
    // Offset to add to the node's world position in camera space to move to the ideal camera
    // location so that scale will work.  This offset should be added *after* translation into
    // camera space
    QVector3D m_worldPosOffset;
    // Position in camera space to center the widget around
    QVector3D m_position;
    // Scale factor to scale the widget by.
    float m_scale = 1.0f;

    // The camera used to render this object.
    QSSGRenderCamera *m_camera = nullptr;
    QSSGWidgetRenderInformation(const QMatrix3x3 &inNormal,
                                  const QMatrix4x4 &inNodeParentToCamera,
                                  const QMatrix4x4 &inLayerProjection,
                                  const QMatrix4x4 &inProjection,
                                  const QMatrix3x3 &inLookAt,
                                  const QMatrix4x4 &inCameraGlobalInverse,
                                  const QVector3D &inWorldPosOffset,
                                  const QVector3D &inPos,
                                  float inScale,
                                  QSSGRenderCamera &inCamera)
        : m_normalMatrix(inNormal)
        , m_nodeParentToCamera(inNodeParentToCamera)
        , m_layerProjection(inLayerProjection)
        , m_pureProjection(inProjection)
        , m_lookAtMatrix(inLookAt)
        , m_cameraGlobalInverse(inCameraGlobalInverse)
        , m_worldPosOffset(inWorldPosOffset)
        , m_position(inPos)
        , m_scale(inScale)
        , m_camera(&inCamera)
    {
    }
    QSSGWidgetRenderInformation() = default;
};
typedef QPair<QSSGShaderVertexCodeGenerator &, QSSGShaderFragmentCodeGenerator &> TShaderGeneratorPair;

enum class RenderWidgetModes
{
    Local,
    Global,
};

class QSSGRenderWidgetInterface
{
public:
    QAtomicInt ref;
    virtual ~QSSGRenderWidgetInterface();
    QSSGRenderNode *m_node = nullptr;

    QSSGRenderWidgetInterface(QSSGRenderNode &inNode) : m_node(&inNode) {}
    QSSGRenderWidgetInterface() = default;
    virtual void render(QSSGRendererImpl &inWidgetContext, QSSGRenderContext &inRenderContext) = 0;
    QSSGRenderNode &getNode() { return *m_node; }

    // Pure widgets.
    static QSSGRef<QSSGRenderWidgetInterface> createBoundingBoxWidget(QSSGRenderNode &inNode,
                                                                          const QSSGBounds3 &inBounds,
                                                                          const QVector3D &inColor);
    static QSSGRef<QSSGRenderWidgetInterface> createAxisWidget(QSSGRenderNode &inNode);
};
QT_END_NAMESPACE

#endif
