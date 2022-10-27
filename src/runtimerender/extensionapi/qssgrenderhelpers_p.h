// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QSSGRENDERHELPERS_P_H
#define QSSGRENDERHELPERS_P_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>

#include <QtCore/qsize.h>

#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGFrameData;
struct QSSGRenderableObject;
class QRhiRenderPassDescriptor;
struct QSSGRhiGraphicsPipelineState;
struct QSSGRenderCamera;
struct QSSGRenderableNodeEntry;

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
                                  const QSSGRenderCamera &camera,
                                  RenderableFilter filter,
                                  float lodThreshold = 0.0f);



private:
    QSSGModelHelpers();
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

QT_END_NAMESPACE

#endif // QSSGRENDERHELPERS_P_H
