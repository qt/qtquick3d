/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtTest>

#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>

class picking : public QObject
{
    Q_OBJECT

public:
    picking();
    ~picking() = default;

private Q_SLOTS:
    void bench_picking1();
    void bench_picking1Miss();
    void bench_picking1in1k();
    void bench_picking1in1kMiss();

private:
    QSSGRef<QSSGRenderContext> renderContext;
    QSSGRef<QSSGInputStreamFactory> inputStreamFactory;
    QSSGRef<QSSGBufferManager> bufferManager;

    void benchImpl(int count, bool hit);
};

picking::picking()
    : renderContext(QSSGRenderContext::createNull())
    , inputStreamFactory(new QSSGInputStreamFactory)
    , bufferManager(new QSSGBufferManager(renderContext, inputStreamFactory, nullptr))
{
}

void picking::bench_picking1()
{
    benchImpl(1, true);
}

void picking::bench_picking1Miss()
{
    benchImpl(1, false);
}

void picking::bench_picking1in1k()
{
    benchImpl(1000, true);
}

void picking::bench_picking1in1kMiss()
{
    benchImpl(1, false);
}

void picking::benchImpl(int count, bool hit)
{
    Q_ASSERT(count > 0 && count <= 1000);
    QSSGRendererImpl renderer(nullptr);
    QVector2D viewportDim(400.0f, 400.0f);
    QSSGRenderLayer dummyLayer;
    QMatrix4x4 globalTransform;

    QSSGRenderCamera dummyCamera;
    dummyCamera.position = QVector3D(0.0f, 0.0f, 600.0f);
    dummyCamera.flags.setFlag(QSSGRenderCamera::Flag::Orthographic);
    dummyCamera.markDirty(QSSGRenderCamera::TransformDirtyFlag::TransformIsDirty);
    dummyCamera.calculateGlobalVariables(QRectF(QPointF(), QSizeF(viewportDim.x(), viewportDim.y())));
    dummyCamera.calculateViewProjectionMatrix(globalTransform);

    dummyLayer.flags.setFlag(QSSGRenderNode::Flag::LayerRenderToTarget, true);
    dummyLayer.renderedCamera = &dummyCamera;

    static const auto setModelPosition = [](QSSGRenderModel &model, const QVector3D &pos) {
        model.position = pos;
        model.markDirty(QSSGRenderModel::TransformDirtyFlag::TransformIsDirty);
        model.calculateGlobalVariables();
    };

    QSSGRenderModel models[1000];

    const auto cubeMeshPath = QSSGRenderMeshPath::create(QStringLiteral("#Cube"));

    for (int i = 0; i != count; ++i) {
        auto &model = models[i];
        model.meshPath = cubeMeshPath;
        model.flags.setFlag(QSSGRenderNode::Flag::LocallyPickable, true);
        setModelPosition(model, { 0.0f + i, 0.0f + i, 0.0f + i });
        dummyLayer.addChild(model);
    }

    // Since we're using the same mesh for each model, we only need to call loadMesh() once.
    bufferManager->loadMesh((*models).meshPath);

    QSSGRenderPickResult res;
    QVector2D mouseCoords = hit ? QVector2D{ 200.0f, 200.0f} : QVector2D{ 0.0f, 0.0f};
    QBENCHMARK {
        res = renderer.syncPick(dummyLayer, bufferManager, viewportDim, { 200.0f, 200.0f});
    }
    QVERIFY(res.m_hitObject != nullptr);
}

QTEST_APPLESS_MAIN(picking)

#include "tst_picking.moc"
