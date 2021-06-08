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

#include <QtQuick3D/private/qquick3dnode_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>

class tst_QQuick3DNode : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class NodeItem : public QQuick3DNode
    {
    public:
        using QQuick3DNode::updateSpatialNode;
    };

private slots:
    void testProperties();
    void testEnums();
    void testPositionMapping();
    void testDirectionMapping();
};

void tst_QQuick3DNode::testProperties()
{
    NodeItem nodeItem;
    auto node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(nullptr));
    const auto originalNode = node; // for comparisons later...
    QVERIFY(node);

    // Positions
    const float x = 50.0f;
    nodeItem.setX(x);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(x, nodeItem.x());
    QCOMPARE(x, node->position.x());

    const float y = 60.0f;
    nodeItem.setY(y);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(y, nodeItem.y());
    QCOMPARE(y, node->position.y());

    const float z = 70.0f;
    nodeItem.setZ(z);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(z, nodeItem.z());
    QCOMPARE(z, node->position.z());

    QVector3D pos1(x, y, z);
    QCOMPARE(pos1, nodeItem.position());
    QCOMPARE(pos1, node->position);

    pos1 *= 2.0;
    nodeItem.setPosition(pos1);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(pos1, nodeItem.position());
    QCOMPARE(pos1, node->position);

    // Rotation, Scale, Pivot
    QCOMPARE(nodeItem.rotation(), QQuaternion());
    QCOMPARE(nodeItem.scale(), QVector3D(1, 1, 1));
    QCOMPARE(nodeItem.pivot(), QVector3D());

    QQuaternion rot = QQuaternion::fromEulerAngles(100, 200, 300);
    nodeItem.setRotation(rot);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(rot, nodeItem.rotation());
    QCOMPARE(rot, node->rotation);

    QVector3D scale(0.5, 1.0, 2.0);
    nodeItem.setScale(scale);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(scale, nodeItem.scale());
    QCOMPARE(scale, node->scale);

    QVector3D pivot(10, 20, 30);
    nodeItem.setPivot(pivot);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(pivot, nodeItem.pivot());
    QCOMPARE(pivot, node->pivot);

    // Opacity
    const float opacity = 0.2f;
    nodeItem.setLocalOpacity(opacity);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QCOMPARE(opacity, nodeItem.localOpacity());
    QCOMPARE(opacity, node->localOpacity);

    // Visibility
    nodeItem.setVisible(false);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QVERIFY(!node->flags.testFlag(QSSGRenderNode::Flag::Active));
    QSignalSpy spy(&nodeItem, SIGNAL(visibleChanged()));
    nodeItem.setVisible(true);
    node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(node));
    QVERIFY(node->flags.testFlag(QSSGRenderNode::Flag::Active));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(originalNode, node);
}

void tst_QQuick3DNode::testEnums()
{
    NodeItem nodeItem;
    auto node = static_cast<QSSGRenderNode *>(nodeItem.updateSpatialNode(nullptr));
    QVERIFY(node);
}

void tst_QQuick3DNode::testPositionMapping()
{
    QQuick3DNode nodeA, nodeB;
    nodeB.setPosition(QVector3D{2.0f, 2.0f, 2.0f});

    { // map pos TO scene A
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapPositionToScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map pos TO scene B
        const QVector3D expected { 3.0f, 3.0f, 3.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeB.mapPositionToScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map pos FROM scene A
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapPositionFromScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map pos FROM scene B
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 3.0f, 3.0f, 3.0f };
        const auto res = nodeB.mapPositionFromScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map node A's postion TO node B
        const QVector3D expected { -1.0f, -1.0f, -1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapPositionToNode(&nodeB, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map node A's postion to node 'null'
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapPositionToNode(nullptr, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map pos FROM node B's pos
        const QVector3D expected { 3.0f, 3.0f, 3.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapPositionFromNode(&nodeB, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map pos FROM null node
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapPositionFromNode(nullptr, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }
}

void tst_QQuick3DNode::testDirectionMapping()
{
    QQuick3DNode nodeA, nodeB;
    nodeB.setEulerRotation(QVector3D{0.0f, 0.0f, 90.0f});

    { // map dir TO scene A
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapDirectionToScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map dir TO scene B
        const QVector3D expected { -1.0f, 1.0f, 0.0f };
        const QVector3D input { 1.0f, 1.0f, 0.0f };
        const auto res = nodeB.mapDirectionToScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map dir FROM scene A
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapDirectionFromScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map dir FROM scene B
        const QVector3D expected { 1.0f, -1.0f, 0.0f };
        const QVector3D input { 1.0f, 1.0f, 0.0f };
        const auto res = nodeB.mapDirectionFromScene(input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map direction FROM node A's TO node B's space
        const QVector3D expected { 1.0f, -1.0f, 0.0f };
        const QVector3D input { 1.0f, 1.0f, 0.0f };
        const auto res = nodeA.mapDirectionToNode(&nodeB, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map direction FROM node A's TO null node
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapDirectionToNode(nullptr, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map direction FROM node B's pos
        const QVector3D expected { -1.0f, 1.0f, 0.0f };
        const QVector3D input { 1.0f, 1.0f, 0.0f };
        const auto res = nodeA.mapDirectionFromNode(&nodeB, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }

    { // map pos FROM null node
        const QVector3D expected { 1.0f, 1.0f, 1.0f };
        const QVector3D input { 1.0f, 1.0f, 1.0f };
        const auto res = nodeA.mapDirectionFromNode(nullptr, input);
        QVERIFY(qFuzzyCompare(res, expected));
    }
}

QTEST_APPLESS_MAIN(tst_QQuick3DNode)
#include "tst_qquick3dnode.moc"
