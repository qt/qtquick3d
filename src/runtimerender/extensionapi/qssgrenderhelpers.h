// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QSSGRENDERHELPERS_H
#define QSSGRENDERHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtQuick3D API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtQuick3DRuntimeRender/qtquick3druntimerenderglobal.h>

#include <ssg/qssgrenderbasetypes.h>

#include <QtCore/qsize.h>

#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGFrameData;
struct QSSGRenderableObject;
class QRhiRenderPassDescriptor;
class QRhiTexture;
struct QSSGRhiGraphicsPipelineState;
struct QSSGRenderCamera;
struct QSSGRenderableNodeEntry;

class QSSGRenderGraphObject;
class QSSGRenderContextInterface;
struct QSSGModelContext;


using QSSGRenderableNodes = QVector<QSSGRenderableNodeEntry>;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGModelHelpers
{
public:
    using RenderableFilter = std::function<bool(QSSGModelContext *)>;

    static void ensureMeshes(const QSSGRenderContextInterface &contextInterface,
                             QSSGRenderableNodes &renderableModels);

    static bool createRenderables(QSSGRenderContextInterface &contextInterface,
                                  const QSSGRenderableNodes &renderableModels,
                                  const QSSGRenderGraphObject &camera,
                                  RenderableFilter filter,
                                  float lodThreshold = 0.0f);



private:
    QSSGModelHelpers();
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGCameraHelpers
{
public:
    static QMatrix4x4 getViewProjectionMatrix(const QSSGRenderGraphObject &camera);

private:
    QSSGCameraHelpers();
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderHelpers
{
public:
    static void rhiPrepareRenderable(QSSGRhiContext &rhiCtx,
                                     QSSGPassKey passKey,
                                     const QSSGFrameData &frameData,
                                     QSSGRenderableObject &inObject,
                                     QRhiRenderPassDescriptor *renderPassDescriptor,
                                     QSSGRhiGraphicsPipelineState *ps,
                                     int samples);

    static void rhiRenderRenderable(QSSGRhiContext &rhiCtx,
                                    const QSSGRhiGraphicsPipelineState &state,
                                    QSSGRenderableObject &object,
                                    bool *needsSetViewport);


private:
    QSSGRenderHelpers();
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderExtensionHelpers
{
public:
    static void registerRenderResult(const QSSGRenderContextInterface &contextInterface,
                                     QSSGExtensionId extension,
                                     QRhiTexture *texture);

private:
    QSSGRenderExtensionHelpers();
};

QT_END_NAMESPACE

#endif // QSSGRENDERHELPERS_H
