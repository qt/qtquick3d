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

#include <QtQuick3DRuntimeRender/private/qssgrenderray_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderhelper_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

class intersection : public QObject
{
    Q_OBJECT

public:
    intersection() = default;
    ~intersection() = default;

private slots:
    void bench_aabbIntersection();
    void bench_aabbIntersectionv2();
};

void intersection::bench_aabbIntersection()
{
    QSSGRenderRay::IntersectionResult res;
    QMatrix4x4 globalTransform; // Identity
    // Bounding box of size 1, placed at origin
    QSSGBounds3 bounds { /*min=*/ QVector3D{ -0.5f, -0.5f, -0.5f }, /*max=*/ QVector3D{ 0.5f, 0.5f, 0.5f } };

    // Pick ray goes directly down the z-axis and should hit directly in the middle of the box
    const auto pickRay = QSSGRenderRay(/*Origin=*/{0.0f, 0.0f, 10.0f}, /*Direction=*/{0.0f, 0.0f, -1.0f});
    res = QSSGRenderRay::intersectWithAABB(globalTransform, bounds, pickRay);
    QCOMPARE(res.intersects, true);

    QBENCHMARK {
        QSSGRenderRay::intersectWithAABB(globalTransform, bounds, pickRay);
    }
}

void intersection::bench_aabbIntersectionv2()
{
    QMatrix4x4 globalTransform; // Identity
    // Bounding box of size 1, placed at origin
    QSSGBounds3 bounds { /*min=*/ QVector3D{ -0.5f, -0.5f, -0.5f }, /*max=*/ QVector3D{ 0.5f, 0.5f, 0.5f } };

    // Pick ray goes directly down the z-axis and should hit directly in the middle of the box
    const auto pickRay = QSSGRenderRay(/*Origin=*/{0.0f, 0.0f, 10.0f}, /*Direction=*/{0.0f, 0.0f, -1.0f});

    QSSGRenderRay::RayData data = QSSGRenderRay::createRayData(globalTransform, pickRay);

    const auto hit = QSSGRenderRay::intersectWithAABBv2(data, bounds);
    QCOMPARE(hit.intersects(), true);

    QBENCHMARK {
        QSSGRenderRay::intersectWithAABBv2(data, bounds);
    }
}

QTEST_APPLESS_MAIN(intersection)

#include "tst_intersection.moc"
