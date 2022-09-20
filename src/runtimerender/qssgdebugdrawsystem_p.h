// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGDEBUGDRAWSYSTEM_H
#define QSSGDEBUGDRAWSYSTEM_H

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
#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtGui/QVector3D>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGDebugDrawSystem
{
    Q_DISABLE_COPY(QSSGDebugDrawSystem)
public:
    QAtomicInt ref;

    QSSGDebugDrawSystem();
    ~QSSGDebugDrawSystem();

    bool hasContent() const;

    void drawLine(const QVector3D &startPoint,
                  const QVector3D &endPoint,
                  const QColor &color,
                  bool isPersistent = false);
    void drawBounds(const QSSGBounds3 &bounds,
                    const QColor &color,
                    bool isPersistent = false);
    void drawPoint(const QVector3D &vertex,
                   const QColor &color,
                   bool isPersistent = false);

    void prepareGeometry(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *rub);
    void recordRenderDebugObjects(QSSGRhiContext *rhiCtx,
                                  QSSGRhiGraphicsPipelineState *ps,
                                  QRhiShaderResourceBindings *srb,
                                  QRhiRenderPassDescriptor *rpDesc);

private:
    struct LineData {
        QVector3D startPoint;
        QVector3D endPoint;
        QColor color;
    };
    struct BoundsData {
        QSSGBounds3 bounds;
        QColor color;
    };
    struct VertexData {
        QVector3D position;
        QVector3D color;
    };


    void generateLine(const LineData &line, QVector<VertexData> &vertexArray, QVector<quint32> &indexArray);
    void generateBox(const BoundsData &bounds, QVector<VertexData> &vertexArray, QVector<quint32> &indexArray);


    quint32 m_indexSize = 0;
    quint32 m_pointsSize = 0;
    QVector<LineData> m_persistentLines;
    QVector<LineData> m_lines;
    QVector<BoundsData> m_persistentBounds;
    QVector<BoundsData> m_bounds;
    QVector<VertexData> m_persistentPoints;
    QVector<VertexData> m_points;

    QSSGRef<QSSGRhiBuffer> m_lineVertexBuffer;
    QSSGRef<QSSGRhiBuffer> m_lineIndexBuffer;
    QSSGRef<QSSGRhiBuffer> m_pointVertexBuffer;
};

QT_END_NAMESPACE

#endif // QSSGDEBUGDRAWSYSTEM_H
