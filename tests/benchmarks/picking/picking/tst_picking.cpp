// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtTest>

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>

class picking : public QObject
{
    Q_OBJECT

public:
    picking();
    ~picking() = default;

private Q_SLOTS:
    void initTestCase();
    void bench_picking1();
    void bench_picking1Miss();
    void bench_picking1in1k();
    void bench_picking1in1kMiss();

private:
    std::unique_ptr<QSSGRhiContext> renderContext;
    std::unique_ptr<QSSGBufferManager> bufferManager;

    void benchImpl(int count, bool hit);
};

picking::picking()
    : renderContext(new QSSGRhiContext)
    , bufferManager(new QSSGBufferManager)
{
}

void picking::initTestCase()
{
    QSKIP("Test does not work with the RHI implementation at the moment");
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
    QSSGRenderer renderer;
    QVector2D viewportDim(400.0f, 400.0f);
    QSSGRenderLayer dummyLayer;
    QMatrix4x4 globalTransform;

    QSSGRenderCamera dummyCamera(QSSGRenderCamera::Type::OrthographicCamera);
    dummyCamera.localTransform.translate(QVector3D(0.0f, 0.0f, 600.0f));
    static_cast<QSSGRenderNode &>(dummyCamera).markDirty(QSSGRenderNode::DirtyFlag::TransformDirty);
    dummyCamera.calculateGlobalVariables(QRectF(QPointF(), QSizeF(viewportDim.x(), viewportDim.y())));
    dummyCamera.calculateViewProjectionMatrix(globalTransform);

    dummyLayer.renderedCamera = &dummyCamera;

    static const auto setModelPosition = [](QSSGRenderModel &model, const QVector3D &pos) {
        model.localTransform.translate(pos);
        model.markDirty(QSSGRenderNode::DirtyFlag::TransformDirty);
        model.calculateGlobalVariables();
    };

    QSSGRenderModel models[1000];

    const auto cubeMeshPath = QSSGRenderPath(QStringLiteral("#Cube"));

    for (int i = 0; i != count; ++i) {
        auto &model = models[i];
        model.meshPath = cubeMeshPath;
        model.setState(QSSGRenderModel::LocalState::Pickable);
        setModelPosition(model, { 0.0f + i, 0.0f + i, 0.0f + i });
        dummyLayer.addChild(model);
    }

    // Since we're using the same mesh for each model, we only need to call loadMesh() once.
    bufferManager->loadMesh(models);

    QSSGRenderPickResult res;
    QSSGRenderRay ray = hit ? QSSGRenderRay{ { 0.0f, 0.0f, -100.0f }, { 0.0f, 0.0f, 1.0f } } : QSSGRenderRay{ { 0.0f, 0.0f, -100.0f }, { 1.0f, 0.0f, 0.0f } };
    QBENCHMARK {
        res = renderer.syncPick(dummyLayer, *bufferManager, ray);
    }
    QVERIFY(res.m_hitObject != nullptr);
}

QTEST_APPLESS_MAIN(picking)

#include "tst_picking.moc"
