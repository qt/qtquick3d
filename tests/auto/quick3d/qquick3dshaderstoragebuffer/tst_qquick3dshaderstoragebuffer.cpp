// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QQuick3DRenderExtension>
#include <QtQuick3D/private/qquick3dshaderstoragebuffer_p.h>
#include <QtQuick3D/private/qquick3dshaderutils_p.h>

#ifdef QT_STATIC
// Static builds need the QML plugins explicitly imported, since the test loads
// "import QtQuick" and "import QtQuick3D" via QQmlComponent::setData() at
// runtime and the import scanner cannot discover them from the C++ sources.
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QQuick3DPlugin)
#endif

// QQuick3DShaderStorageBufferData is QML_UNCREATABLE, but can be instantiated from C++.
class TestBufferData : public QQuick3DShaderStorageBufferData
{
public:
    void setBytes(const QByteArray &bytes) { setBufferData(bytes); }
};

class tst_QQuick3DShaderStorageBuffer : public QObject
{
    Q_OBJECT

private slots:
    void testCreation();
    void testSizeProperty();
    void testBufferProviderProperty();
    void testBufferDataProperty();
};

void tst_QQuick3DShaderStorageBuffer::testCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderStorageBuffer { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    auto *buffer = qobject_cast<QQuick3DShaderStorageBuffer *>(object.data());
    QVERIFY(buffer != nullptr);
    QCOMPARE(buffer->size(), quint32(0));
    QCOMPARE(buffer->bufferProvider(), nullptr);
    QCOMPARE(buffer->bufferData(), nullptr);
}

void tst_QQuick3DShaderStorageBuffer::testSizeProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderStorageBuffer { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *buffer = qobject_cast<QQuick3DShaderStorageBuffer *>(object.data());
    QVERIFY(buffer != nullptr);

    QSignalSpy spy(buffer, &QQuick3DShaderStorageBuffer::sizeChanged);
    buffer->setSize(256);
    QCOMPARE(buffer->size(), quint32(256));
    QCOMPARE(spy.size(), 1);

    // Setting the same value again is a no-op.
    buffer->setSize(256);
    QCOMPARE(spy.size(), 1);

    buffer->setSize(0);
    QCOMPARE(buffer->size(), quint32(0));
    QCOMPARE(spy.size(), 2);
}

void tst_QQuick3DShaderStorageBuffer::testBufferProviderProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderStorageBuffer { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *buffer = qobject_cast<QQuick3DShaderStorageBuffer *>(object.data());
    QVERIFY(buffer != nullptr);

    QQuick3DRenderExtension extension;
    QSignalSpy spy(buffer, &QQuick3DShaderStorageBuffer::bufferProviderChanged);

    buffer->setBufferProvider(&extension);
    QCOMPARE(buffer->bufferProvider(), &extension);
    QCOMPARE(spy.size(), 1);

    // No-op when set to the same provider again.
    buffer->setBufferProvider(&extension);
    QCOMPARE(spy.size(), 1);

    buffer->setBufferProvider(nullptr);
    QCOMPARE(buffer->bufferProvider(), nullptr);
    QCOMPARE(spy.size(), 2);
}

void tst_QQuick3DShaderStorageBuffer::testBufferDataProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderStorageBuffer { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *buffer = qobject_cast<QQuick3DShaderStorageBuffer *>(object.data());
    QVERIFY(buffer != nullptr);

    TestBufferData data;
    data.setBytes(QByteArrayLiteral("hello"));

    QSignalSpy dataSpy(buffer, &QQuick3DShaderStorageBuffer::bufferDataChanged);
    QSignalSpy sizeSpy(buffer, &QQuick3DShaderStorageBuffer::sizeChanged);

    buffer->setBufferData(&data);
    QCOMPARE(buffer->bufferData(), static_cast<QQuick3DShaderStorageBufferData *>(&data));
    QCOMPARE(dataSpy.size(), 1);
    // Assigning bufferData immediately syncs size to the data's byte count.
    QCOMPARE(buffer->size(), quint32(5));
    QCOMPARE(sizeSpy.size(), 1);

    // The buffer tracks live changes to the data via bufferDataDirty().
    data.setBytes(QByteArrayLiteral("hello world!"));
    QCOMPARE(buffer->size(), quint32(12));
    QCOMPARE(sizeSpy.size(), 2);

    // Clearing bufferData resets size back to 0.
    buffer->setBufferData(nullptr);
    QCOMPARE(buffer->bufferData(), nullptr);
    QCOMPARE(dataSpy.size(), 2);
    QCOMPARE(buffer->size(), quint32(0));
    QCOMPARE(sizeSpy.size(), 3);
}

QTEST_MAIN(tst_QQuick3DShaderStorageBuffer)
#include "tst_qquick3dshaderstoragebuffer.moc"
