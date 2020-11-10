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

#include <QtCore/qfile.h>
#include <QtCore/qbytearray.h>

#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>

#include <parser.h>

class Shadergen : public QObject
{
    Q_OBJECT
public:
    Shadergen() = default;
    ~Shadergen() = default;

private Q_SLOTS:
    void initTestCase();
    void tst_principledMaterialComponent();
    void tst_defaultMaterialComponent();
    void tst_customMaterialComponent();
    void tst_customMaterialUniforms_data();
    void tst_customMaterialUniforms();

private:
    MaterialParser::SceneData customMaterialSceneData;
};

void Shadergen::initTestCase()
{
}

void Shadergen::tst_principledMaterialComponent()
{
    QFile file(QLatin1String(":/qml/PrincipledMaterialA.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();

    MaterialParser::SceneData sceneData;
    MaterialParser::parseQmlData(data, file.fileName(), sceneData);

    QCOMPARE(sceneData.materials.size(), 1);
    QQuick3DPrincipledMaterial *mat = qobject_cast<QQuick3DPrincipledMaterial *>(sceneData.materials.at(0).ptr);
    QVERIFY(qFuzzyCompare(mat->metalness(), 0.1f));
    QVERIFY(qFuzzyCompare(mat->roughness(), 0.2f));
    QCOMPARE(mat->baseColor(), QColor("green"));
}

void Shadergen::tst_defaultMaterialComponent()
{
    QFile file(QLatin1String(":/qml/DefaultMaterialA.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();

    MaterialParser::SceneData sceneData;
    MaterialParser::parseQmlData(data, file.fileName(), sceneData);

    QCOMPARE(sceneData.materials.size(), 1);
    QQuick3DDefaultMaterial *mat = qobject_cast<QQuick3DDefaultMaterial *>(sceneData.materials.at(0).ptr);
    QVERIFY(qFuzzyCompare(mat->specularAmount(), 0.1f));
    QVERIFY(qFuzzyCompare(mat->specularRoughness(), 0.2f));
    QCOMPARE(mat->diffuseColor(), QColor("pink"));
}

void Shadergen::tst_customMaterialComponent()
{
    QFile file(QLatin1String(":/qml/CustomMaterialA.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();

    auto &sceneData = customMaterialSceneData;
    MaterialParser::parseQmlData(data, file.fileName(), sceneData);

    QCOMPARE(sceneData.materials.size(), 1);
    QQuick3DCustomMaterial *mat = qobject_cast<QQuick3DCustomMaterial *>(sceneData.materials.at(0).ptr);
    QCOMPARE(mat->shadingMode(), QQuick3DCustomMaterial::ShadingMode::Unshaded);
}

void Shadergen::tst_customMaterialUniforms_data()
{
    QTest::addColumn<QMetaType>("type");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("bool0") << QMetaType(QMetaType::Bool) << "uBoolFalse" << QVariant::fromValue(false);
    QTest::newRow("bool1") << QMetaType(QMetaType::Bool) << "uBoolTrue" << QVariant::fromValue(true);
    QTest::newRow("int") << QMetaType(QMetaType::Double) /*Not a typo*/ << "uInt" << QVariant::fromValue(int(33));
    QTest::newRow("real") << QMetaType(QMetaType::Double) << "uReal" << QVariant::fromValue(qreal(3.3));
    QTest::newRow("pointS") << QMetaType(QMetaType::QPointF) << "uPointS" << QVariant::fromValue(QPointF(0, 1));
    QTest::newRow("pointF") << QMetaType(QMetaType::QPointF) << "uPointF" << QVariant::fromValue(QPointF(1, 0));
    QTest::newRow("sizeS") << QMetaType(QMetaType::QSizeF) << "uSizeS" << QVariant::fromValue(QSizeF(1, 1));
    QTest::newRow("sizeF") << QMetaType(QMetaType::QSizeF) << "uSizeF" << QVariant::fromValue(QSizeF(2, 2));
    QTest::newRow("rectS") << QMetaType(QMetaType::QRectF) << "uRectS" << QVariant::fromValue(QRectF(0, 1, 100, 101));
    QTest::newRow("rectF") << QMetaType(QMetaType::QRectF) << "uRectF" << QVariant::fromValue(QRectF(1, 0, 101, 100));
    QTest::newRow("vec2") << QMetaType(QMetaType::QVector2D) << "uVec2" << QVariant::fromValue(QVector2D(1, 2));
    QTest::newRow("vec3") << QMetaType(QMetaType::QVector3D) << "uVec3" << QVariant::fromValue(QVector3D(1, 2, 3));
    QTest::newRow("vec4") << QMetaType(QMetaType::QVector4D) << "uVec4" << QVariant::fromValue(QVector4D(1, 2, 3, 4));
    QTest::newRow("quat") << QMetaType(QMetaType::QQuaternion) << "uQuat" << QVariant::fromValue(QQuaternion(1, 2, 3, 4));
    QTest::newRow("m44") << QMetaType(QMetaType::QMatrix4x4) << "uM44"
                         << QVariant::fromValue(QMatrix4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    QTest::newRow("Color1") << QMetaType(QMetaType::QColor) << "uColor1" << QVariant::fromValue(QColor("green"));
    QTest::newRow("Color2") << QMetaType(QMetaType::QColor) << "uColor2" << QVariant::fromValue(QColor("#ff0000"));
    QTest::newRow("Texture") << QMetaType(qMetaTypeId<QQuick3DTexture *>()) << "uTex"
                             << QVariant::fromValue((QQuick3DTexture *)nullptr);
    QTest::newRow("TextureInput") << QMetaType(qMetaTypeId<QQuick3DShaderUtilsTextureInput *>()) << "uTexInput"
                                  << QVariant::fromValue((QQuick3DShaderUtilsTextureInput *)nullptr);
}

void Shadergen::tst_customMaterialUniforms()
{
    // Test that public QML members are added to the object
    const auto &sceneData = customMaterialSceneData;
    QCOMPARE(sceneData.materials.size(), 1);
    QQuick3DCustomMaterial *mat = qobject_cast<QQuick3DCustomMaterial *>(sceneData.materials.at(0).ptr);
    QCOMPARE(mat->shadingMode(), QQuick3DCustomMaterial::ShadingMode::Unshaded);

    QFETCH(QMetaType, type);
    QFETCH(QString, name);
    QFETCH(QVariant, value);

    const auto prop = mat->property(name.toLatin1().constData());
    QVERIFY(prop.isValid());
    QCOMPARE(prop.metaType(), type);
    if (type == QMetaType(QMetaType::Double)) {
        QVERIFY(qFuzzyCompare(prop.toDouble(), value.toDouble()));
    } else if (type.id() >= QMetaType::User) {
        const bool ok = (prop.metaType().id() == qMetaTypeId<QQuick3DTexture *>()) ||
                (prop.metaType().id() == qMetaTypeId<QQuick3DShaderUtilsTextureInput *>());
        if (!ok) {
            qDebug() << "Texture: " << qMetaTypeId<QQuick3DTexture *>();
            qDebug() << "TextureInput: " << qMetaTypeId<QQuick3DShaderUtilsTextureInput *>();
            qDebug() << prop.metaType().id();
        }
        QVERIFY(ok);
    } else {
        QCOMPARE(prop, value);
    }
}

QTEST_APPLESS_MAIN(Shadergen)

#include "tst_shadergen.moc"
