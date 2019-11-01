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
#include <QtQuick3D/private/qquick3dcustomcamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

class tst_QQuick3DCustomCamera : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class Camera : public QQuick3DCustomCamera
    {
    public:
        using QQuick3DCamera::updateSpatialNode;
    };

private slots:
    void testCustomProjection();
};

void tst_QQuick3DCustomCamera::testCustomProjection()
{
    Camera camera;
    auto node = static_cast<QSSGRenderCamera *>(camera.updateSpatialNode(nullptr));
    const auto originalNode = node; // for comparisons later...
    QVERIFY(node);

    // clang-format off
    const QMatrix4x4 customProjection = {
        2, 0, 0, 0,
        0, 4, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 1,
    };
    // clang-format on
    camera.setProjection(customProjection);
    node = static_cast<QSSGRenderCamera *>(camera.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QVERIFY(node->flags.testFlag(QSSGRenderNode::Flag::CameraDirty));
    QCOMPARE(customProjection, node->projection);
}

QTEST_APPLESS_MAIN(tst_QQuick3DCustomCamera)
#include "tst_qquick3dcustomcamera.moc"
