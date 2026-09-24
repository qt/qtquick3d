// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <QtQuick3DHelpers/private/capsulegeometry_p.h>
#include <QtQuick3DHelpers/private/conegeometry_p.h>
#include <QtQuick3DHelpers/private/cuboidgeometry_p.h>
#include <QtQuick3DHelpers/private/cylindergeometry_p.h>
#include <QtQuick3DHelpers/private/extrudedtextgeometry_p.h>
#include <QtQuick3DHelpers/private/planegeometry_p.h>
#include <QtQuick3DHelpers/private/spheregeometry_p.h>
#include <QtQuick3DHelpers/private/torusgeometry_p.h>

#include <functional>

using GeometryFactory = std::function<QQuick3DGeometry *()>;
Q_DECLARE_METATYPE(GeometryFactory)

class tst_ProceduralGeometry : public QObject
{
    Q_OBJECT

private slots:
    void regenerate_data();
    void regenerate();
};

void tst_ProceduralGeometry::regenerate_data()
{
    QTest::addColumn<GeometryFactory>("create");
    QTest::addColumn<QByteArray>("property");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<bool>("asynchronous");

    struct Row
    {
        const char *type;
        GeometryFactory create;
        const char *property;
        QVariant value;
    };
    const Row rows[] = {
        { "SphereGeometry", [] { return new SphereGeometry; }, "radius", 50.0f },
        { "CuboidGeometry", [] { return new CuboidGeometry; }, "xExtent", 50.0f },
        { "ConeGeometry", [] { return new ConeGeometry; }, "length", 50.0f },
        { "CylinderGeometry", [] { return new CylinderGeometry; }, "radius", 20.0f },
        { "TorusGeometry", [] { return new TorusGeometry; }, "radius", 50.0f },
        { "PlaneGeometry", [] { return new PlaneGeometry; }, "width", 50.0f },
        { "CapsuleGeometry", [] { return new CapsuleGeometry; }, "height", 50.0f },
        { "ExtrudedTextGeometry",
          [] {
              auto *geometry = new ExtrudedTextGeometry;
              geometry->setText(QStringLiteral("Qt"));
              return geometry;
          },
          "depth",
          5.0f },
    };

    for (const Row &row : rows) {
        for (bool async : { false, true }) {
            QTest::addRow("%s-%s", row.type, async ? "async" : "sync")
                    << row.create << QByteArray(row.property) << row.value << async;
        }
    }
}

void tst_ProceduralGeometry::regenerate()
{
    QFETCH(GeometryFactory, create);
    QFETCH(QByteArray, property);
    QFETCH(QVariant, value);
    QFETCH(bool, asynchronous);

    QScopedPointer<QQuick3DGeometry> geometry(create());
    QVERIFY(geometry->setProperty("asynchronous", asynchronous));

    QSignalSpy spy(geometry.data(), &QQuick3DGeometry::geometryChanged);
    QTRY_COMPARE(spy.size(), 1);
    const int attributeCount = geometry->attributeCount();
    QVERIFY(attributeCount > 0);
    QVERIFY(!geometry->vertexData().isEmpty());

    for (int i = 0; i < 3; ++i) {
        QVERIFY(geometry->setProperty(property.constData(), value.toFloat() * (i + 3)));
        QTRY_COMPARE(spy.size(), i + 2);
        QCOMPARE(geometry->attributeCount(), attributeCount);
        QVERIFY(!geometry->vertexData().isEmpty());
    }
}

QTEST_MAIN(tst_ProceduralGeometry)
#include "tst_proceduralgeometry.moc"
