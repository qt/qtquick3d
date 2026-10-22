// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3D/private/qquick3dshaderutils_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>

#ifdef QT_STATIC
// Static builds need the QML plugins explicitly imported, since the test loads
// "import QtQuick3D" via QQmlComponent::setData() at runtime and the import
// scanner cannot discover it from the C++ sources.
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QQuick3DPlugin)
#endif

// Minimal QQuick3DPropertyChangedTracker owner, the same setup RenderPass and
// SkyMaterial use, so the tracked property list can be inspected directly.
class TrackedOwner : public QQuick3DObject, public QQuick3DPropertyChangedTracker
{
    Q_OBJECT
public:
    explicit TrackedOwner(QQuick3DObject *parent = nullptr)
        : QQuick3DObject(parent)
        , QQuick3DPropertyChangedTracker(this, QQuick3DSuperClassInfo<TrackedOwner>())
    {
    }

    QByteArrayList trackedNames()
    {
        QByteArrayList names;
        for (const auto &property : extractProperties())
            names.append(property.name);
        return names;
    }

protected:
    void markTrackedPropertyDirty(QMetaProperty, DirtyPropertyHint) override { }
};

class tst_QQuick3DShaderUtils : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testTrackedReferenceDestroyed();
    void testTrackedReferenceReassigned();
};

void tst_QQuick3DShaderUtils::initTestCase()
{
    qmlRegisterType<TrackedOwner>("TrackerTest", 1, 0, "TrackedOwner");
}

static constexpr char trackedOwnerQml[] = R"(
import QtQuick3D
import TrackerTest
TrackedOwner {
    property real first: 1.0
    property Texture second
    property real third: 3.0
})";

void tst_QQuick3DShaderUtils::testTrackedReferenceDestroyed()
{
    // Destroying a referenced texture marks its property dirty twice, once from
    // QML nulling the property and once from the tracker's destroyed handler.
    // The second entry must not remove the property that follows it.
    QQmlEngine engine;
    QQuick3DSceneManager sceneManager;
    QQmlComponent component(&engine);
    component.setData(trackedOwnerQml, QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<TrackedOwner> owner(qobject_cast<TrackedOwner *>(component.create()));
    QVERIFY(owner);
    QQuick3DObjectPrivate::get(owner.data())->refSceneManager(sceneManager);

    QScopedPointer<QQuick3DTexture> texture(new QQuick3DTexture);
    owner->setProperty("second", QVariant::fromValue(texture.data()));
    QCOMPARE(owner->trackedNames(), QByteArrayList({ "first", "second", "third" }));

    texture.reset();
    QCOMPARE(owner->property("second").value<QQuick3DTexture *>(), nullptr);
    QCOMPARE(owner->trackedNames(), QByteArrayList({ "first", "third" }));
}

void tst_QQuick3DShaderUtils::testTrackedReferenceReassigned()
{
    // A property that went null is no longer in the list. Assigning a texture
    // to it again must re-insert it, not overwrite the property that follows it.
    QQmlEngine engine;
    QQuick3DSceneManager sceneManager;
    QQmlComponent component(&engine);
    component.setData(trackedOwnerQml, QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<TrackedOwner> owner(qobject_cast<TrackedOwner *>(component.create()));
    QVERIFY(owner);
    QQuick3DObjectPrivate::get(owner.data())->refSceneManager(sceneManager);

    QScopedPointer<QQuick3DTexture> texture1(new QQuick3DTexture);
    QScopedPointer<QQuick3DTexture> texture2(new QQuick3DTexture);
    owner->setProperty("second", QVariant::fromValue(texture1.data()));
    QCOMPARE(owner->trackedNames(), QByteArrayList({ "first", "second", "third" }));

    owner->setProperty("second", QVariant::fromValue<QQuick3DTexture *>(nullptr));
    QCOMPARE(owner->trackedNames(), QByteArrayList({ "first", "third" }));

    owner->setProperty("second", QVariant::fromValue(texture2.data()));
    QCOMPARE(owner->trackedNames(), QByteArrayList({ "first", "second", "third" }));

    // The first texture is still watched, destroying it must not drop the
    // property that now refers to the second one.
    texture1.reset();
    QCOMPARE(owner->trackedNames(), QByteArrayList({ "first", "second", "third" }));
}

QTEST_MAIN(tst_QQuick3DShaderUtils)
#include "tst_qquick3dshaderutils.moc"
