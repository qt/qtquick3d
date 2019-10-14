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

#include <QtQuick3D/private/qquick3dlight_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3D/private/qquick3dobject_p_p.h>

class tst_QQuick3DLight : public QObject
{
    Q_OBJECT

    // Work-around to get access to updateSpatialNode
    class Light : public QQuick3DLight
    {
    public:
        using QQuick3DLight::updateSpatialNode;
    };

private slots:
    void testProperties();
    void testEnums();
    void testScope();
};

void tst_QQuick3DLight::testProperties()
{
    Light light;
    auto node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(nullptr));
    const auto originalNode = node; // for comparisons later...
    QVERIFY(node);

    const float brightness = 50.0f;
    light.setBrightness(brightness);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(brightness, node->m_brightness);
    QCOMPARE(light.brightness(), node->m_brightness);

    const float linearFade = 0.4f;
    light.setLinearFade(linearFade);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(linearFade, node->m_linearFade);
    QCOMPARE(light.linearFade(), node->m_linearFade);

    const float exponentialFade = 0.4f;
    light.setExponentialFade(exponentialFade);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(exponentialFade, node->m_exponentialFade);
    QCOMPARE(light.exponentialFade(), node->m_exponentialFade);

    const float areaWidth = 200.0f;
    light.setAreaWidth(areaWidth);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(areaWidth, node->m_areaWidth);
    QCOMPARE(light.areaWidth(), node->m_areaWidth);

    const float areaHeight = 200.0f;
    light.setAreaHeight(areaHeight);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(areaHeight, node->m_areaHeight);
    QCOMPARE(light.areaHeight(), node->m_areaHeight);

    const float shadowBias = 0.5f;
    light.setShadowBias(shadowBias);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(shadowBias, node->m_shadowBias);
    QCOMPARE(light.shadowBias(), node->m_shadowBias);

    const float shadowFactor = 4.0f;
    light.setShadowFactor(shadowFactor);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(shadowFactor, node->m_shadowFactor);
    QCOMPARE(light.shadowFactor(), node->m_shadowFactor);

    const float shadowMapFar = 2000.0f;
    light.setShadowMapFar(shadowMapFar);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(shadowMapFar, node->m_shadowMapFar);
    QCOMPARE(light.shadowMapFar(), node->m_shadowMapFar);

    const float shadowMapFieldOfView = 80.0f;
    light.setShadowMapFieldOfView(shadowMapFieldOfView);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(shadowMapFieldOfView, node->m_shadowMapFov);
    QCOMPARE(light.shadowMapFieldOfView(), node->m_shadowMapFov);

    const float shadowFilter = 20.0f;
    light.setShadowFilter(shadowFilter);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(shadowFilter, node->m_shadowFilter);
    QCOMPARE(light.shadowFilter(), node->m_shadowFilter);

    const int shadowMapResolution = 8;
    light.setShadowMapResolution(shadowMapResolution);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(shadowMapResolution, node->m_shadowMapRes);
    QCOMPARE(light.shadowMapResolution(), node->m_shadowMapRes);

    light.setCastShadow(true);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QVERIFY(node->m_castShadow);
    light.setCastShadow(false);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QVERIFY(!node->m_castShadow);

    QColor color1("#12345678");
    QVector3D color1Vec3(float(color1.redF()), float(color1.greenF()),
                         float(color1.blueF()));
    QColor color2("#cccccccc");
    QVector3D color2Vec3(float(color2.redF()), float(color2.greenF()),
                         float(color2.blueF()));
    light.setDiffuseColor(color1);
    light.setSpecularColor(color2);
    light.setAmbientColor(color1);
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    QCOMPARE(originalNode, node);
    QCOMPARE(color1, light.diffuseColor());
    QCOMPARE(color2, light.specularColor());
    QCOMPARE(color1, light.ambientColor());
    // Note: none of these colors contain alpha
    QCOMPARE(color1Vec3, node->m_diffuseColor);
    QCOMPARE(color2Vec3, node->m_specularColor);
    QCOMPARE(color1Vec3, node->m_ambientColor);
}

void tst_QQuick3DLight::testEnums()
{
    Light light;
    auto node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(nullptr));
    QVERIFY(node);

    // These test rely on the different enums having the same values
    auto lightModes = { QQuick3DLight::QSSGRenderLightTypes::Unknown,
                        QQuick3DLight::QSSGRenderLightTypes::Directional,
                        QQuick3DLight::QSSGRenderLightTypes::Point,
                        QQuick3DLight::QSSGRenderLightTypes::Area };
    for (const auto lightMode : lightModes)
    {
        light.setLightType(lightMode);
        node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(nullptr));
        QCOMPARE(int(light.lightType()), int(node->m_lightType));
    }
}

void tst_QQuick3DLight::testScope()
{
    Light light;
    auto node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(nullptr));
    QVERIFY(node);
    QQuick3DNode nodeItem;
    QVERIFY(!light.scope());
    light.setScope(&nodeItem);
    QVERIFY(light.scope());
    node = static_cast<QSSGRenderLight *>(light.updateSpatialNode(node));
    auto scope = static_cast<QSSGRenderNode*>(QQuick3DObjectPrivate::get(light.scope())->spatialNode);
    QCOMPARE(scope, node->m_scope);
}

QTEST_APPLESS_MAIN(tst_QQuick3DLight)
#include "tst_qquick3dlight.moc"
