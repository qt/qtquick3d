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

#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>

class tst_QQuick3DMaterials : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class Material : public QQuick3DDefaultMaterial
    {
    public:
        using QQuick3DMaterial::updateSpatialNode;
    };

    class CustomMaterial : public QQuick3DCustomMaterial
    {
    public:
        using QQuick3DMaterial::updateSpatialNode;
    };

    class Texture : public QQuick3DTexture
    {
    public:
        using QQuick3DTexture::updateSpatialNode;
    };

private slots:
    void testProperties();
    void testTextures();
    void testEnums();
    void testCustomMaterials();
};

void tst_QQuick3DMaterials::testProperties()
{
    Material material;
    auto node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(nullptr));
    const auto originalNode = node; // for comparisons later...
    QVERIFY(node);

    const float opacity = 0.4f;
    material.setOpacity(opacity);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(material.opacity(), node->opacity);

    const float displacementAmount = 0.5f;
    material.setDisplacementAmount(displacementAmount);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(material.displacementAmount(), node->displaceAmount);

    const float specularAmount = 10.0f;
    material.setSpecularAmount(specularAmount);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(material.specularAmount(), node->specularAmount);

    const float emissivePower = 0.1f;
    material.setEmissivePower(emissivePower);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(material.emissivePower(), node->emissivePower);

    const float bumpAmount = 0.3f;
    material.setBumpAmount(bumpAmount);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(material.bumpAmount(), node->bumpAmount);

    material.setVertexColors(true);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(material.vertexColors(), node->vertexColors);
    material.setVertexColors(false);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(material.vertexColors(), node->vertexColors);

    QColor color1("#12345678");
    QVector4D color1Vec4(float(color1.redF()), float(color1.greenF()),
                         float(color1.blueF()), float(color1.alphaF()));
    material.setDiffuseColor(color1);
    material.setEmissiveColor(color1);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(color1Vec4, node->color);
    // Note: emissiveColor doesn't contain alpha
    QCOMPARE(QVector3D(color1Vec4), node->emissiveColor);

}

void tst_QQuick3DMaterials::testTextures()
{
    Material material;
    auto node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(nullptr));
    const auto originalNode = node; // for comparisons later...
    QVERIFY(node);

    Texture texture1;
    texture1.setSource(QUrl(":/Built_with_Qt.png"));
    Texture texture2;
    texture2.setSource(QUrl(":/Built_with_Qt_2.png"));

    QQuick3DSceneManager sceneManager;
    sceneManager.updateDirtyNode(&texture1);
    sceneManager.updateDirtyNode(&texture2);

    // Diffusemaps
    material.setDiffuseMap(&texture1);
    material.setDiffuseMap2(&texture1);
    material.setDiffuseMap3(&texture2);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QVERIFY(texture1.getRenderImage());
    QVERIFY(texture2.getRenderImage());
    QCOMPARE(texture1.getRenderImage(), node->colorMaps[QSSGRenderDefaultMaterial::DiffuseColor0]);
    QVERIFY(node->colorMaps[QSSGRenderDefaultMaterial::DiffuseColor0] ==
            node->colorMaps[QSSGRenderDefaultMaterial::DiffuseColor1]);
    QVERIFY(node->colorMaps[QSSGRenderDefaultMaterial::DiffuseColor0] !=
            node->colorMaps[QSSGRenderDefaultMaterial::DiffuseColor2]);

    // Specularmaps
    QVERIFY(!node->specularMap);
    QVERIFY(!node->specularReflection);
    QVERIFY(!material.specularMap());
    QVERIFY(!material.specularReflectionMap());
    material.setSpecularMap(&texture2);
    material.setSpecularReflectionMap(&texture1);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QVERIFY(material.specularMap());
    QCOMPARE(material.specularMap()->getRenderImage(), node->specularMap);
    QVERIFY(material.specularReflectionMap());
    QCOMPARE(material.specularReflectionMap()->getRenderImage(), node->specularReflection);

    // Bumpmap
    QVERIFY(!node->bumpMap);
    QVERIFY(!material.bumpMap());
    material.setBumpMap(&texture1);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QVERIFY(material.bumpMap());
    QCOMPARE(material.bumpMap()->getRenderImage(), node->bumpMap);

    // Opacitymap
    QVERIFY(!node->opacityMap);
    QVERIFY(!material.opacityMap());
    material.setOpacityMap(&texture1);
    node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(node));
    QVERIFY(material.opacityMap());
    QCOMPARE(material.opacityMap()->getRenderImage(), node->opacityMap);
    QCOMPARE(material.opacityMap()->getRenderImage(), material.bumpMap()->getRenderImage());
}

void tst_QQuick3DMaterials::testEnums()
{
    Material material;
    auto node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(nullptr));
    QVERIFY(node);

    // These test rely on the different enums having the same values
    auto lightModes = { QQuick3DDefaultMaterial::QSSGDefaultMaterialLighting::NoLighting,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialLighting::VertexLighting,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialLighting::FragmentLighting };
    for (const auto lightMode : lightModes)
    {
        material.setLighting(lightMode);
        node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(nullptr));
        QCOMPARE(int(material.lighting()), int(node->lighting));
    }

    auto blendModes = { QQuick3DDefaultMaterial::QSSGDefaultMaterialBlendMode::Normal,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialBlendMode::Screen,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialBlendMode::Overlay,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialBlendMode::Multiply,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialBlendMode::ColorBurn,
                        QQuick3DDefaultMaterial::QSSGDefaultMaterialBlendMode::ColorDodge };
    for (const auto blendMode : blendModes)
    {
        material.setBlendMode(blendMode);
        node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(nullptr));
        QCOMPARE(int(material.blendMode()), int(node->blendMode));
    }

    auto specularModes = { QQuick3DDefaultMaterial::QSSGDefaultMaterialSpecularModel::Default,
                           QQuick3DDefaultMaterial::QSSGDefaultMaterialSpecularModel::KGGX,
                           QQuick3DDefaultMaterial::QSSGDefaultMaterialSpecularModel::KWard };
    for (const auto specularMode : specularModes)
    {
        material.setSpecularModel(specularMode);
        node = static_cast<QSSGRenderDefaultMaterial *>(material.updateSpatialNode(nullptr));
        QCOMPARE(int(material.specularModel()), int(node->specularModel));
    }
}

void tst_QQuick3DMaterials::testCustomMaterials()
{
    CustomMaterial material;
    QQuick3DCustomMaterialShaderInfo shaderInfo;
    shaderInfo.version = "1.2.3";
    QQuick3DViewport *view3D = new QQuick3DViewport;
    material.setParent(view3D);
    material.setShaderInfo(&shaderInfo);
    auto node = static_cast<QSSGRenderCustomMaterial *>(material.updateSpatialNode(nullptr));
    QVERIFY(node);
    QVERIFY(material.shaderInfo()->version == node->shaderInfo.version);

    QQuick3DCustomMaterialTexture mTexture;
    QQuick3DTexture qTexture;
    mTexture.type = QQuick3DCustomMaterialTexture::TextureType::Bump;
    QSignalSpy spy(&mTexture, SIGNAL(textureDirty(QQuick3DCustomMaterialTexture *)));
    mTexture.setImage(&qTexture);
    QCOMPARE(spy.count(), 1);
    QVERIFY(&qTexture == mTexture.image());

    QQuick3DCustomMaterialBuffer mBuffer;
    auto format = QQuick3DCustomMaterialBuffer::TextureFormat::RGBA8;
    mBuffer.setFormat(format);
    auto filterOp = QQuick3DCustomMaterialBuffer::MagnifyingOp::Nearest;
    mBuffer.setFilterOp(filterOp);
    QCOMPARE(format, mBuffer.format());
    QCOMPARE(filterOp, mBuffer.filterOp());

    // TODO: Extend custom material testing with proper shaders

    view3D->deleteLater();
}

QTEST_APPLESS_MAIN(tst_QQuick3DMaterials)
#include "tst_qquick3dmaterials.moc"
