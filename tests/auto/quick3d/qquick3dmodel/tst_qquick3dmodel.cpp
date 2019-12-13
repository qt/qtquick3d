/****************************************************************************
**
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

#include <QTest>
#include <QSignalSpy>

#include <QtQuick3D/private/qquick3dmodel_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>

class tst_QQuick3DModel : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class Model : public QQuick3DModel
    {
    public:
        using QQuick3DModel::updateSpatialNode;
    };

private slots:
    void testProperties();
    void testEnums();
};

void tst_QQuick3DModel::testProperties()
{
    Model model;
    auto node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(nullptr));
    const auto originalNode = node; // for comparisons later...
    QVERIFY(node);

    const float edgeTess = 0.2f;
    model.setEdgeTessellation(edgeTess);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QCOMPARE(edgeTess, model.edgeTessellation());
    QCOMPARE(edgeTess, node->edgeTessellation);

    const float innerTess = 0.3f;
    model.setInnerTessellation(innerTess);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QCOMPARE(innerTess, model.innerTessellation());
    QCOMPARE(innerTess, node->innerTessellation);

    model.setIsWireframeMode(true);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(model.isWireframeMode());
    QVERIFY(node->wireframeMode);
    model.setIsWireframeMode(false);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(!model.isWireframeMode());
    QVERIFY(!node->wireframeMode);

    model.setCastsShadows(true);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(model.castsShadows());
    QVERIFY(node->castsShadows);
    model.setCastsShadows(false);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(!model.castsShadows());
    QVERIFY(!node->castsShadows);

    model.setReceivesShadows(true);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(model.receivesShadows());
    QVERIFY(node->receivesShadows);
    model.setReceivesShadows(false);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(!model.receivesShadows());
    QVERIFY(!node->receivesShadows);

    model.setPickable(true);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(model.pickable());
    QVERIFY(node->flags.testFlag(QSSGRenderModel::Flag::LocallyPickable));
    model.setPickable(false);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QVERIFY(!model.receivesShadows());
    QVERIFY(!node->flags.testFlag(QSSGRenderModel::Flag::LocallyPickable));

    // mesh from source
    QUrl cubeUrl("#Cube");
    QSignalSpy spy(&model, SIGNAL(sourceChanged()));
    model.setSource(cubeUrl);
    QCOMPARE(spy.count(), 1);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QCOMPARE(cubeUrl, model.source());
    QCOMPARE(cubeUrl, node->meshPath.path);
    QCOMPARE(originalNode, node);

    QQuick3DGeometry geometry;
    model.setGeometry(&geometry);
    node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
    QCOMPARE(&geometry, model.geometry());
}

void tst_QQuick3DModel::testEnums()
{
    Model model;
    auto node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(nullptr));
    QVERIFY(node);

    auto tessModes = { QQuick3DModel::QSSGTessellationModeValues::NoTessellation,
                       QQuick3DModel::QSSGTessellationModeValues::Linear,
                       QQuick3DModel::QSSGTessellationModeValues::Phong,
                       QQuick3DModel::QSSGTessellationModeValues::NPatch };
    for (const auto tessMode : tessModes) {
        model.setTessellationMode(tessMode);
        node = static_cast<QSSGRenderModel *>(model.updateSpatialNode(node));
        QCOMPARE(int(model.tessellationMode()), int(node->tessellationMode));
    }
}

QTEST_APPLESS_MAIN(tst_QQuick3DModel)
#include "tst_qquick3dmodel.moc"
