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

#ifndef QSSG_RENDER_PATH_MANAGER_H
#define QSSG_RENDER_PATH_MANAGER_H

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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpath_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendersubpath_p.h>
#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE

struct QSSGLayerGlobalRenderProperties;
class QSSGPathManagerInterface;

struct QSSGPathAnchorPoint
{
    QVector2D position;
    float incomingAngle;
    float outgoingAngle;
    float incomingDistance;
    float outgoingDistance;
};

struct QSSGPathRenderContext; // UICRenderPathRenderContext.h

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGPathManagerInterface
{
public:
    QAtomicInt ref;
    // returns the path buffer id
    //!! Note this call is made from multiple threads simultaneously during binary load.
    //!! - see UICRenderGraphObjectSerializer.cpp
    virtual void setPathSubPathData(const QSSGRenderSubPath &inPathSubPath,
                                    QSSGDataView<QSSGPathAnchorPoint> inPathSubPathAnchorPoints) = 0;

    virtual ~QSSGPathManagerInterface();
    virtual QSSGDataRef<QSSGPathAnchorPoint> getPathSubPathBuffer(const QSSGRenderSubPath &inPathSubPath) = 0;
    // Marks the PathSubPath anchor points as dirty.  This will mean rebuilding any PathSubPath
    // context required to render the PathSubPath.
    virtual QSSGDataRef<QSSGPathAnchorPoint> resizePathSubPathBuffer(const QSSGRenderSubPath &inPathSubPath,
                                                                         quint32 inNumAnchors) = 0;
    virtual QSSGBounds3 getBounds(const QSSGRenderPath &inPath) = 0;

    // Helper functions used in various locations
    // Angles here are in degrees because that is how they are represented in the data.
    static QVector2D getControlPointFromAngleDistance(QVector2D inPosition, float inAngle, float inDistance);

    // Returns angle in x, distance in y.
    static QVector2D getAngleDistanceFromControlPoint(QVector2D inPosition, QVector2D inControlPoint);

    static QSSGRef<QSSGPathManagerInterface> createPathManager(QSSGRenderContextInterface *inContext);

    // The path segments are next expected to change after this call; changes will be ignored.
    virtual bool prepareForRender(const QSSGRenderPath &inPath) = 0;

    virtual void renderDepthPrepass(QSSGPathRenderContext &inRenderContext,
                                    QSSGLayerGlobalRenderProperties inRenderProperties,
                                    TShaderFeatureSet inFeatureSet) = 0;

    virtual void renderShadowMapPass(QSSGPathRenderContext &inRenderContext,
                                     QSSGLayerGlobalRenderProperties inRenderProperties,
                                     TShaderFeatureSet inFeatureSet) = 0;

    virtual void renderCubeFaceShadowPass(QSSGPathRenderContext &inRenderContext,
                                          QSSGLayerGlobalRenderProperties inRenderProperties,
                                          TShaderFeatureSet inFeatureSet) = 0;

    virtual void renderPath(QSSGPathRenderContext &inRenderContext,
                            QSSGLayerGlobalRenderProperties inRenderProperties,
                            TShaderFeatureSet inFeatureSet) = 0;
};
QT_END_NAMESPACE
#endif
