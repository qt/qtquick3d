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
#include <QtQuick/qquickitem.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>

class tst_QQuick3DTexture : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class Texture : public QQuick3DTexture
    {
    public:
        using QQuick3DTexture::updateSpatialNode;
    };

private slots:
    void testSetSource();
    void testSetSourceItem();
    void testMappingAndTilingModes();
    void testTransformations();
    void testFormat();
};

void tst_QQuick3DTexture::testSetSource()
{
    Texture texture;
    std::unique_ptr<QSSGRenderImage> node;

    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QVERIFY(node);
    // check that we get the same node out when passing in the old one
    QCOMPARE(node.get(), texture.updateSpatialNode(node.get()));

    QSignalSpy spy(&texture, SIGNAL(sourceChanged()));
    QCOMPARE(spy.count(), 0);

    const QUrl fileWithScheme {QString::fromLatin1("file:path/to/resource")};
    const QString expectedPath {QString::fromLatin1("path/to/resource")};
    texture.setSource(fileWithScheme);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(expectedPath, node->m_imagePath);
    QCOMPARE(spy.count(), 1);

    // Same url again
    texture.setSource(fileWithScheme);
    QCOMPARE(spy.count(), 1);
}

void tst_QQuick3DTexture::testSetSourceItem()
{
    Texture texture;

    QQuick3DViewport view3D;
    QQuickItem item;
    item.setParentItem(&view3D);

    qRegisterMetaType<QQuickItem *>();
    QSignalSpy spy(&texture, SIGNAL(sourceItemChanged()));
    QCOMPARE(spy.count(), 0);

    QVERIFY(!texture.sourceItem());
    texture.setSourceItem(&item);
    QVERIFY(texture.sourceItem());
    QCOMPARE(spy.count(), 1);

    // Same item again
    texture.setSourceItem(&item);
    QCOMPARE(spy.count(), 1);
}

void tst_QQuick3DTexture::testMappingAndTilingModes()
{
    Texture texture;
    std::unique_ptr<QSSGRenderImage> node;

    // This test relies on the two different enums (QQ3DT::MappingModes & QSSGRI::MappingModes)
    // having the same values
    for (const auto mappingMode : {QQuick3DTexture::UV, QQuick3DTexture::LightProbe, QQuick3DTexture::Environment })
    {
        texture.setMappingMode(mappingMode);
        node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
        QCOMPARE(int(mappingMode), int(node->m_mappingMode));
    }

    // This also relies on the two enums (QQ3DT::TilingMode & QSSGRenderTextureCoordOp)
    // having the same values
    for (const auto tilingMode : {QQuick3DTexture::ClampToEdge, QQuick3DTexture::MirroredRepeat, QQuick3DTexture::Repeat })
    {
        texture.setHorizontalTiling(tilingMode);
        node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
        QCOMPARE(tilingMode, int(node->m_horizontalTilingMode));

        texture.setVerticalTiling(tilingMode);
        node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
        QCOMPARE(tilingMode, int(node->m_verticalTilingMode));
    }
}

void tst_QQuick3DTexture::testTransformations()
{
    Texture texture;
    std::unique_ptr<QSSGRenderImage> node;

    const float scaleU = 0.7f;
    texture.setScaleU(scaleU);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(scaleU, node->m_scale.x());

    const float scaleV = 0.36f;
    texture.setScaleV(scaleV);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(scaleV, node->m_scale.y());

    const float rotationUV = 0.38f;
    texture.setRotationUV(rotationUV);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(rotationUV, node->m_rotation);

    const float positionU = 0.39f;
    texture.setPositionU(positionU);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(positionU, node->m_position.x());

    const float positionV = 0.40f;
    texture.setPositionV(positionV);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(positionV, node->m_position.y());

    const float pivotU = 0.41f;
    texture.setPivotU(pivotU);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(pivotU, node->m_pivot.x());

    const float pivotV = 0.42f;
    texture.setPivotV(pivotV);
    node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
    QCOMPARE(pivotV, node->m_pivot.y());
}

void tst_QQuick3DTexture::testFormat()
{
    Texture texture;
    std::unique_ptr<QSSGRenderImage> node;

    // This test _also_ relies on two enums having the same values
    // (QQ3DT::Format & QSSGRenderTextureFormat::Format).
    auto metaEnum = QMetaEnum::fromType<QQuick3DTexture::Format>();
    for (int i = 0; i < metaEnum.keyCount(); i++) {
        const auto format = QQuick3DTexture::Format(metaEnum.value(i));

        texture.setFormat(format);
        node.reset(static_cast<QSSGRenderImage *>(texture.updateSpatialNode(nullptr)));
        QCOMPARE(int(format), int(node->m_format.format));
    }
}


QTEST_APPLESS_MAIN(tst_QQuick3DTexture)
#include "tst_qquick3dtexture.moc"
