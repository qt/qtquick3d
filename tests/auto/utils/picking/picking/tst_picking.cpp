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
    void test_picking();

private:
    QSSGRef<QSSGRenderContext> renderContext;
    QSSGRef<QSSGInputStreamFactory> inputStreamFactory;
    QSSGRef<QSSGBufferManager> bufferManager;
};

picking::picking()
    : renderContext(QSSGRenderContext::createNull())
    , inputStreamFactory(new QSSGInputStreamFactory)
    , bufferManager(new QSSGBufferManager(renderContext, inputStreamFactory, nullptr))
{
}

void picking::test_picking()
{
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
    QSSGRenderModel model1, model2;
    model1.flags.setFlag(QSSGRenderNode::Flag::LocallyPickable, true);
    model2.flags.setFlag(QSSGRenderNode::Flag::LocallyPickable, true);
    model1.meshPath = model2.meshPath = QSSGRenderMeshPath::create(QStringLiteral("#Cube"));
    // Since we're using the same mesh for each model, we only need to call loadMesh() once.
    bufferManager->loadMesh(model1.meshPath);

    // position
    // NOTE: Camera is special, so don't use this for the camera!
    static const auto setModelPosition = [](QSSGRenderModel &model, const QVector3D &pos) {
        model.position = pos;
        model.markDirty(QSSGRenderModel::TransformDirtyFlag::TransformIsDirty);
        model.calculateGlobalVariables();
    };
    setModelPosition(model1, { 0.0f, 0.0f, 0.0f });
    setModelPosition(model2, { 50.0f, 50.0f, -50.0f });

    dummyLayer.addChild(model1);
    dummyLayer.addChild(model2);

    // Center of model1
    auto res = renderer.syncPick(dummyLayer, bufferManager, viewportDim, {200.0f, 200.0f});
    QVERIFY(res.m_hitObject != nullptr);
    QCOMPARE(res.m_hitObject, &model1);

    // Upper right corner of model1
    res = renderer.syncPick(dummyLayer, bufferManager, viewportDim, {250.0f, 150.0f});
    QVERIFY(res.m_hitObject != nullptr);
    QCOMPARE(res.m_hitObject, &model1);

    // Just outside model1's upper right corner, so should hit the model behind (model2)
    res = renderer.syncPick(dummyLayer, bufferManager, viewportDim, {251.0f, 151.0f});
    QVERIFY(res.m_hitObject != nullptr);
    QCOMPARE(res.m_hitObject, &model2);

    // Upper right corner of model2
    res = renderer.syncPick(dummyLayer, bufferManager, viewportDim, {300.0f, 100.0f});
    QVERIFY(res.m_hitObject != nullptr);
    QCOMPARE(res.m_hitObject, &model2);

    // Just outside model2's upper right corner, so there should be no hit
    res = renderer.syncPick(dummyLayer, bufferManager, viewportDim, {301.0f, 99.0f});
    QCOMPARE(res.m_hitObject, nullptr);
}

QTEST_APPLESS_MAIN(picking)

#include "tst_picking.moc"
