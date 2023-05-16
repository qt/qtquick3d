// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QSignalSpy>
#include <QTest>
#include <QtQuick/QQuickItem>

#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>
#include <QtQuick3D/private/qquick3ditem2d_p.h>

#include "../shared/util.h"

class CustomGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    QML_NAMED_ELEMENT(CustomGeometry)

public:
    CustomGeometry(uint count);

    int count() { return m_count; }
    void setCount(int count);

signals:
    void countChanged();

private:
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D uv;
    };

    void updateData();

    uint m_count = 500;
};


CustomGeometry::CustomGeometry(uint count)
{
    m_count = count;
    updateData();
}

void CustomGeometry::setCount(int count)
{
    m_count = count;
    updateData();
    emit countChanged();
}

void CustomGeometry::updateData()
{
    QVector<Vertex> vertices;
    QVector<QPoint> coords;

    int rowCount = m_count;
    int columnCount = m_count;
    float uvX = 1.0f / float(rowCount - 1);
    float uvY = 1.0f / float(columnCount - 1);

    for (int i = 0; i < columnCount; i++) {
        for (int j = 0; j < rowCount; j++) {
            Vertex v;
            float x = 4.0f / (columnCount-1) * i - 2.0f;
            float z = 4.0f / (rowCount-1) * j - 2.0f;
            v.position = QVector3D(x, .0f, z);
            v.normal = QVector3D(.0f, .0f, .0f);
            v.uv = QVector2D(j * uvX, i * uvY);
            vertices.push_back(v);
            coords.push_back(QPoint(i, j));
        }
    }
    QVector3D boundsMin = QVector3D(-2.0f, -1.0f, -2.0f);
    QVector3D boundsMax = QVector3D(2.0f, 1.0f, 2.0f);
    setBounds(boundsMin, boundsMax);

    QVector<quint32> indices;
    for (int i = 0; i < columnCount - 1; i++) {
        for (int j = 0; j < rowCount -1; j++) {
            indices.push_back(i * rowCount + j);
            indices.push_back(i * rowCount + j + 1);
            indices.push_back(i * rowCount + j + rowCount);

            indices.push_back(i * rowCount + j + 1);
            indices.push_back(i * rowCount + j + 1 + rowCount);
            indices.push_back(i * rowCount + j + rowCount);
        }
    }

    setStride(sizeof(Vertex));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                           0,
                           QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,
                           sizeof(QVector3D) * 2,
                           QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                           sizeof(QVector3D),
                           QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                           0,
                           QQuick3DGeometry::Attribute::U32Type);

    QByteArray vertexBuffer(reinterpret_cast<char *>(vertices.data()), vertices.size() * sizeof(Vertex));
    setVertexData(vertexBuffer);

    QByteArray indexBuffer(reinterpret_cast<char *>(indices.data()), indices.size() * sizeof(quint32));
    setIndexData(indexBuffer);

    update();
}


class tst_Picking : public QQuick3DDataTest
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() override;
    void test_object_picking();
    void test_object_picking2();
    void test_picking_QTBUG_111997();
    void test_picking_corner_case();
    void test_triangleIntersect();

private:
    QQuickItem *find2DChildIn3DNode(QQuickView *view, const QString &objectName, const QString &itemName);

};

void tst_Picking::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

QQuickItem *tst_Picking::find2DChildIn3DNode(QQuickView *view, const QString &objectName, const QString &itemName)
{
    QQuick3DNode *obj = view->rootObject()->findChild<QQuick3DNode*>(objectName);
    if (!obj)
        return nullptr;
    QQuickItem *subsceneRoot = obj->findChild<QQuickItem *>();
    if (!subsceneRoot)
        subsceneRoot = obj->findChild<QQuick3DItem2D *>()->contentItem();
    if (!subsceneRoot)
        return nullptr;
    QObject *child = subsceneRoot->findChild<QObject *>(itemName);
    return static_cast<QQuickItem *>(child);
}

void tst_Picking::test_object_picking()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("picking.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QQuick3DViewport *view3d = view->findChild<QQuick3DViewport *>(QStringLiteral("view"));
    QVERIFY(view3d);
    QQuick3DModel *model1 = view3d->findChild<QQuick3DModel *>(QStringLiteral("model1"));
    QVERIFY(model1);
    QQuick3DModel *model2 = view3d->findChild<QQuick3DModel *>(QStringLiteral("model2"));
    QVERIFY(model2);
    QQuick3DModel *instancedModel = view3d->findChild<QQuick3DModel *>(QStringLiteral("instancedModel"));
    QVERIFY(instancedModel);
    QQuickItem *item2d = qmlobject_cast<QQuickItem *>(find2DChildIn3DNode(view.data(), "item2dNode", "item2d"));
    QVERIFY(item2d);

    const qreal dpr = view->devicePixelRatio();
    if (dpr != 1.0) {
        QSKIP("Test uses window positions to get exact values and those assume DPR of 1.0");
    }

    // Pick nearest based on viewport position

    // Center of model1
    auto result = view3d->pick(200, 200);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Upper right corner of model1
    result = view3d->pick(250, 150);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Just outside model1's upper right corner, so should hit the model behind (model2)
    result = view3d->pick(251, 151);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model2);

    // Upper right corner of model2
    result = view3d->pick(300, 100);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model2);

    // Just outside model2's upper right corner, so there should be no hit
    result = view3d->pick(301, 99);
    QCOMPARE(result.objectHit(), nullptr);

    // Center of the third entry in the instance table
    result = view3d->pick(350, 200);
    QCOMPARE(result.objectHit(), instancedModel);
    QCOMPARE(result.instanceIndex(), 2);

    // Enable the 2D item
    item2d->setEnabled(true);
    QTest::qWait(100);
    // Then picking on top of model1 should not pick it anymore
    result = view3d->pick(200, 200);
    QVERIFY(result.objectHit() != model1);
    // Hide the 2D item
    item2d->setVisible(false);
    QTest::qWait(100);
    // Then picking on top of model1 should pick it again
    result = view3d->pick(200, 200);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Pick all based on viewport position

    // Center of model1, bottom left corner of model2
    auto resultList = view3d->pickAll(200, 200);
    QCOMPARE(resultList.size(), 2);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);

    // Top right corner of model1, center of model2
    resultList = view3d->pickAll(250, 150);
    QCOMPARE(resultList.size(), 2);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);

    // Just outside model1's upper right corner, so should hit the model behind (model2)
    resultList = view3d->pickAll(251, 151);
    QCOMPARE(resultList.size(), 1);
    QCOMPARE(resultList[0].objectHit(), model2);

    // The bottom right of the second entry in the instance table,
    // overlapping model1 and model2
    resultList = view3d->pickAll(225, 175);
    QCOMPARE(resultList.size(), 3);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);
    QCOMPARE(resultList[2].objectHit(), instancedModel);
    QCOMPARE(resultList[2].instanceIndex(), 1);

    // Just outside model2's upper right corner, so there should be no hit
    resultList = view3d->pickAll(301, 99);
    QVERIFY(resultList.isEmpty());

    // Ray based picking one result

    // Down the z axis from 0,0,100 (towards 0,0,0)
    QVector3D origin(0.0f, 0.0f, 100.0f);
    QVector3D direction(0.0f, 0.0f, -1.0f);
    result = view3d->rayPick(origin, direction);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Up the z axis from 0, 0, -101 (towards 0,0,0)
    origin = QVector3D(0.0f, 0.0f, -101.0f);
    direction = QVector3D(0.0f, 0.0f, 1.0f);
    result = view3d->rayPick(origin, direction);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model2);

    // Up the z axis from 0, 0, -100 (towards 0,0,-1000)
    origin = QVector3D(0.0f, 0.0f, -100.0f);
    direction = QVector3D(0.0f, 0.0f, -1.0f);
    result = view3d->rayPick(origin, direction);
    QVERIFY(result.objectHit() == nullptr);

    // Ray based picking all results

    // Down the z axis from 0,0,100 (towards 0,0,0)
    origin = QVector3D(0.0f, 0.0f, 100.0f);
    direction = QVector3D(0.0f, 0.0f, -1.0f);
    resultList = view3d->rayPickAll(origin, direction);
    QCOMPARE(resultList.size(), 2);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);

    // Up the z axis from 0, 0, -101 (towards 0,0,0)
    origin = QVector3D(0.0f, 0.0f, -101.0f);
    direction = QVector3D(0.0f, 0.0f, 1.0f);
    resultList = view3d->rayPickAll(origin, direction);
    QCOMPARE(resultList.size(), 2);
    QCOMPARE(resultList[0].objectHit(), model2);
    QCOMPARE(resultList[1].objectHit(), model1);

    // Up the z axis from 0, 0, -100 (towards 0,0,-1000)
    origin = QVector3D(0.0f, 0.0f, -100.0f);
    direction = QVector3D(0.0f, 0.0f, -1.0f);
    resultList = view3d->rayPickAll(origin, direction);
    QVERIFY(resultList.isEmpty());
}

void tst_Picking::test_object_picking2()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("picking2.qml"), QSize(100, 100)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const qreal dpr = view->devicePixelRatio();
    if (dpr != 1.0) {
        QSKIP("Test uses window positions to get exact values and those assume DPR of 1.0");
    }

    const auto viewSize = view->size();
    const float halfWidth = viewSize.width() * 0.5f;
    const float halfHeight = viewSize.height() * 0.5f;
    const float horizontalPickLine = halfHeight - 35;

    QQuick3DViewport *view3d = view->findChild<QQuick3DViewport *>(QStringLiteral("view"));
    QVERIFY(view3d);
    QQuick3DModel *coneModel = view3d->findChild<QQuick3DModel *>(QStringLiteral("model3"));
    QVERIFY(coneModel);

    // just left of the cone (model3)
    auto result = view3d->pick(halfWidth - 11, horizontalPickLine);
    QCOMPARE(result.objectHit(), nullptr);

    // center top of the cone (model3)
    result = view3d->pick(halfWidth, horizontalPickLine);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), coneModel);

    // just right of the cone (model3)
    result = view3d->pick(halfWidth + 11, horizontalPickLine);
    QCOMPARE(result.objectHit(), nullptr);
}

// When triangles become so small the picking algorithm fails because of floating point
// precision. This test generates a quad with count * count subdivisions leading to smaller
// and smaller triangles to pick against. Even if a mesh is very dense, the triangle intersection
// code should still work and return model1 as a hit.
void tst_Picking::test_picking_QTBUG_111997()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("qtbug_111997.qml"), QSize(100, 100)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QQuick3DViewport *view3d = view->findChild<QQuick3DViewport *>(QStringLiteral("view"));
    QVERIFY(view3d);
    QQuick3DModel *model1 = view3d->findChild<QQuick3DModel *>(QStringLiteral("model1"));
    QVERIFY(model1);

    const auto viewSize = view->size();
    const float halfWidth = viewSize.width() * 0.5f;
    const float halfHeight = viewSize.height() * 0.5f;
    QSignalSpy frameSwappedSpy(view.data(), &QQuickWindow::frameSwapped);

    // Add custom geometry to the scene
    QScopedPointer<CustomGeometry> geometry(new CustomGeometry(20));
    geometry->setParent(model1);
    geometry->setParentItem(model1);
    model1->setGeometry(geometry.data());

    // Wait for the next frame (so that geometry is up-to-date
    if (!frameSwappedSpy.wait())
        QFAIL("frameSwapped() signal not emitted");

    // Check picking with 20 subdivisions
    auto result = view3d->pick(halfWidth, halfHeight);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // check picking with 350 subdivisions
    geometry->setCount(350);
    if (!frameSwappedSpy.wait())
        QFAIL("frameSwapped() signal not emitted");
    result = view3d->pick(halfWidth, halfHeight);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // check picking with 500 subdivisions
    geometry->setCount(500);
    if (!frameSwappedSpy.wait())
        QFAIL("frameSwapped() signal not emitted");
    result = view3d->pick(halfWidth, halfHeight);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

}

void tst_Picking::test_picking_corner_case()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("corner_case.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QQuick3DViewport *view3d = view->findChild<QQuick3DViewport *>(QStringLiteral("view"));
    QVERIFY(view3d);
    QQuick3DModel *model1 = view3d->findChild<QQuick3DModel *>(QStringLiteral("model1"));
    QVERIFY(model1);

    const QVector3D direction = QVector3D(0.0f, 0.0f, 1.0f);

    {
        // bottom right corner (vertex)
        const QVector3D origin1 = QVector3D(0.0f, 0.0f, -101.0f);
        const QVector3D origin2 = QVector3D(0.0f, 0.0f, -100.0f);
        auto result = view3d->rayPick(origin1, direction);
        QVERIFY(result.objectHit() != nullptr);
        QCOMPARE(result.objectHit(), model1);
        QCOMPARE(result.scenePosition(), origin2);
        QCOMPARE(result.normal(), QVector3D(0.0f, 0.0f, -1.0f));
        QCOMPARE(result.uvPosition(), QVector2D(1.0f, 0.0f));

        // The ray doesn't intersect the triangle as the origin
        // starts on the triangle

        // The ray intersects with the other face of the cube,
        // but unless we test for backfaces the test should still

        result = view3d->rayPick(origin2, direction);
        QVERIFY(result.objectHit() == nullptr);
    }

    {
        // top right corner (vertex)
        const QVector3D origin1 = QVector3D(0.0f, 100.0f, -101.0f);
        const QVector3D origin2 = QVector3D(0.0f, 100.0f, -100.0f);
        auto result = view3d->rayPick(origin1, direction);
        QVERIFY(result.objectHit() != nullptr);
        QCOMPARE(result.objectHit(), model1);
        QCOMPARE(result.scenePosition(), origin2);
        QCOMPARE(result.normal(), QVector3D(0.0f, 0.0f, -1.0f));
        QCOMPARE(result.uvPosition(), QVector2D(1.0f, 1.0f));

        // The ray doesn't intersect the triangle as the origin
        // starts on the triangle

        // The ray intersects with the other face of the cube,
        // but unless we test for backfaces the test should still

        result = view3d->rayPick(origin2, direction);
        QVERIFY(result.objectHit() == nullptr);
    }

    {
        // bottom left corner (vertex)
        const QVector3D origin1 = QVector3D(100.0f, 0.0f, -101.0f);
        const QVector3D origin2 = QVector3D(100.0f, 0.0f, -100.0f);
        auto result = view3d->rayPick(origin1, direction);
        QVERIFY(result.objectHit() != nullptr);
        QCOMPARE(result.objectHit(), model1);
        QCOMPARE(result.scenePosition(), origin2);
        QCOMPARE(result.normal(), QVector3D(0.0f, 0.0f, -1.0f));
        QCOMPARE(result.uvPosition(), QVector2D(0.0f, 0.0f));

        // The ray doesn't intersect the triangle as the origin
        // starts on the triangle

        // The ray intersects with the other face of the cube,
        // but unless we test for backfaces the test should still

        result = view3d->rayPick(origin2, direction);
        QVERIFY(result.objectHit() == nullptr);
    }

    {
        // top left corner (vertex)
        const QVector3D origin1 = QVector3D(100.0f, 100.0f, -101.0f);
        const QVector3D origin2 = QVector3D(100.0f, 100.0f, -100.0f);
        auto result = view3d->rayPick(origin1, direction);
        QVERIFY(result.objectHit() != nullptr);
        QCOMPARE(result.objectHit(), model1);
        QCOMPARE(result.scenePosition(), origin2);
        QCOMPARE(result.normal(), QVector3D(0.0f, 0.0f, -1.0f));
        QCOMPARE(result.uvPosition(), QVector2D(0.0f, 1.0f));

        // The ray doesn't intersect the triangle as the origin
        // starts on the triangle

        // The ray intersects with the other face of the cube,
        // but unless we test for backfaces the test should still

        result = view3d->rayPick(origin2, direction);
        QVERIFY(result.objectHit() == nullptr);
    }

    {
        // Center
        const QVector3D origin1 = QVector3D(50.0f, 50.0f, -101.0f);
        const QVector3D origin2 = QVector3D(50.0f, 50.0f, -100.0f);
        auto result = view3d->rayPick(origin1, direction);
        QVERIFY(result.objectHit() != nullptr);
        QCOMPARE(result.objectHit(), model1);
        QCOMPARE(result.scenePosition(), origin2);
        QCOMPARE(result.normal(), QVector3D(0.0f, 0.0f, -1.0f));
        QCOMPARE(result.uvPosition(), QVector2D(0.5f, 0.5f));

        // The ray doesn't intersect the triangle as the origin
        // starts on the triangle

        // The ray intersects with the other face of the cube,
        // but unless we test for backfaces the test should still

        result = view3d->rayPick(origin2, direction);
        QVERIFY(result.objectHit() == nullptr);
    }

}

void tst_Picking::test_triangleIntersect()
{
    const QVector3D direction(0, 0, -1);
    const QVector3D expectedNormal(0, 0, 1);
    const QVector3D vert0(0, 0, 0);
    const QVector3D vert1(-1, -1, 0);
    const QVector3D vert2(1, -1, 0);

    QVector3D normal;
    float u;
    float v;
    bool intersects;
    const QVector3D origin1(0, 0, 2);

    const QSSGRenderRay ray1(origin1, direction); // intersection vert0

    intersects = QSSGRenderRay::triangleIntersect(ray1,
                                                  vert0,
                                                  vert1,
                                                  vert2,
                                                  u,
                                                  v,
                                                  normal);
    QVERIFY(intersects);
    QCOMPARE(normal, expectedNormal);
    QCOMPARE(u, 0.0f);
    QCOMPARE(v, 0.0f);

    const QVector3D origin2(-1, -1, 2);
    const QSSGRenderRay ray2(origin2, direction); // intersection vert1

    intersects = QSSGRenderRay::triangleIntersect(ray2,
                                                  vert0,
                                                  vert1,
                                                  vert2,
                                                  u,
                                                  v,
                                                  normal);
    QVERIFY(intersects);
    QCOMPARE(normal, expectedNormal);
    QCOMPARE(u, 1.0f);
    QCOMPARE(v, 0.0f);

    const QVector3D origin3(1, -1, 2);
    const QSSGRenderRay ray3(origin3, direction); // intersection vert2
    intersects = QSSGRenderRay::triangleIntersect(ray3,
                                                  vert0,
                                                  vert1,
                                                  vert2,
                                                  u,
                                                  v,
                                                  normal);
    QVERIFY(intersects);
    QCOMPARE(normal, expectedNormal);
    QCOMPARE(u, 0.0f);
    QCOMPARE(v, 1.0f);
}

QTEST_MAIN(tst_Picking)
#include "tst_picking.moc"
