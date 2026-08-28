// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <QtCore/qfile.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qtemporarydir.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick3D/private/qquick3dcomputepass_p.h>
#include <QtQuick3D/private/qquick3dshaderutils_p.h>
#include <QtQuick3D/private/qquick3dshaderimage_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3D/private/qquick3dshaderstoragebuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercomputepass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffer_p.h>

#ifdef QT_STATIC
// Static builds need the QML plugins explicitly imported, since the test loads
// "import QtQuick" and "import QtQuick3D" via QQmlComponent::setData() at
// runtime and the import scanner cannot discover them from the C++ sources.
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QQuick3DPlugin)
#endif

namespace {

QUrl writeShader(const QTemporaryDir &dir, const QString &name, const QByteArray &source)
{
    QFile f(dir.filePath(name));
    if (!f.open(QIODevice::WriteOnly))
        return {};
    f.write(source);
    return QUrl::fromLocalFile(f.fileName());
}

QByteArray computeShader(const QByteArray &imageFormat)
{
    QByteArray source(R"(SHADER_IMAGE("@format@", myImage, image2D, "writeonly");
STORAGE_BUFFER(myBuffer, "readonly") { vec4 values[]; };
void MAIN()
{
    imageStore(myImage, ivec2(0), values[0]);
}
)");
    return source.replace("@format@", imageFormat);
}

const char *computePassQml = R"(
import QtQuick3D
ComputePass {
    property ShaderImage myImage: null
    property ShaderStorageBuffer myBuffer: null
})";

template<typename T>
T *createResource(QQmlEngine &engine, const QByteArray &qml)
{
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    if (component.isError())
        qWarning() << component.errorString();
    return qobject_cast<T *>(component.create());
}

// Creates the backend node, as the scene manager would when syncing the object.
QSSGRenderGraphObject *createNode(QQuick3DObject *object)
{
    auto *d = QQuick3DObjectPrivate::get(object);
    d->spatialNode = QQuick3DObjectPrivate::updateSpatialNode(object, nullptr);
    return d->spatialNode;
}

// The scene manager in these tests has no window attachment, so it cannot take
// over backend nodes: QQuick3DSceneManager::cleanupNodes() hands any node with
// graphics resources to the attachment unconditionally. An object that leaves
// the scene, or is destroyed, while it still has its node would put the node
// there, so the node is released first: before the object is swapped out of
// the pass, and before the pass is destroyed, which is why the guards doing it
// are declared after the pass.
void releaseNode(QQuick3DObject *object)
{
    auto *d = QQuick3DObjectPrivate::get(object);
    delete d->spatialNode;
    d->spatialNode = nullptr;
}

const QSSGBaseTypeProperty *findUniform(const QSSGUserComputeShader &shader, const QByteArray &name)
{
    for (const auto &prop : shader.propertyUniforms) {
        if (prop.name == name)
            return &prop;
    }
    return nullptr;
}

} // namespace

class tst_QQuick3DComputePass : public QObject
{
    Q_OBJECT

private slots:
    void testCreation();
    void testComputeShaderProperty();
    void testCommandsListManagement();
    void testDispatchCommand();
    void testDispatchCommandRejectsInvalidGroupCounts();
    void testNullResourcePropertiesAssignedLater();
    void testResourceSwap();
};

void tst_QQuick3DComputePass::testCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ComputePass { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    auto *pass = qobject_cast<QQuick3DComputePass *>(object.data());
    QVERIFY(pass != nullptr);
    QCOMPARE(pass->computeShader(), QUrl());

    auto list = pass->commands();
    QCOMPARE(list.count(&list), 0);
}

void tst_QQuick3DComputePass::testComputeShaderProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ComputePass { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DComputePass *>(object.data());
    QVERIFY(pass != nullptr);

    QSignalSpy spy(pass, &QQuick3DComputePass::computeShaderChanged);
    const QUrl shaderUrl("compute_shader.comp");

    pass->setComputeShader(shaderUrl);
    QCOMPARE(pass->computeShader(), shaderUrl);
    QCOMPARE(spy.size(), 1);

    // Setting the same URL again is a no-op.
    pass->setComputeShader(shaderUrl);
    QCOMPARE(spy.size(), 1);
}

void tst_QQuick3DComputePass::testCommandsListManagement()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick3D
ComputePass {
    commands: [
        Dispatch { groupCountX: 1 },
        Dispatch { groupCountX: 2 }
    ]
})",
                       QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DComputePass *>(object.data());
    QVERIFY(pass != nullptr);

    auto list = pass->commands();
    QCOMPARE(list.count(&list), 2);
    QVERIFY(list.at(&list, 0) != nullptr);
    QVERIFY(list.at(&list, 1) != nullptr);

    list.clear(&list);
    QCOMPARE(list.count(&list), 0);
}

void tst_QQuick3DComputePass::testDispatchCommand()
{
    QQmlEngine engine;

    // Defaults
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; Dispatch { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        auto *dispatch = qobject_cast<QQuick3DShaderUtilsCommandDispatch *>(object.data());
        QVERIFY(dispatch != nullptr);
        QCOMPARE(dispatch->groupCountX(), 1);
        QCOMPARE(dispatch->groupCountY(), 1);
        QCOMPARE(dispatch->groupCountZ(), 1);
    }

    // Explicit group counts, embedded in a ComputePass's commands list, and
    // verify cloneCommand() bridges the values to the backend QSSGDispatch.
    {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick3D
ComputePass {
    commands: [ Dispatch { groupCountX: 4; groupCountY: 2; groupCountZ: 3 } ]
})",
                           QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        auto *pass = qobject_cast<QQuick3DComputePass *>(object.data());
        QVERIFY(pass != nullptr);

        auto list = pass->commands();
        QCOMPARE(list.count(&list), 1);
        auto *dispatch = qobject_cast<QQuick3DShaderUtilsCommandDispatch *>(list.at(&list, 0));
        QVERIFY(dispatch != nullptr);
        QCOMPARE(dispatch->groupCountX(), 4);
        QCOMPARE(dispatch->groupCountY(), 2);
        QCOMPARE(dispatch->groupCountZ(), 3);

        QScopedPointer<QSSGCommand> cloned(dispatch->cloneCommand());
        QVERIFY(cloned != nullptr);
        QCOMPARE(cloned->m_type, CommandType::Dispatch);
        auto *clonedDispatch = static_cast<QSSGDispatch *>(cloned.data());
        QCOMPARE(clonedDispatch->m_groupCount.x, 4);
        QCOMPARE(clonedDispatch->m_groupCount.y, 2);
        QCOMPARE(clonedDispatch->m_groupCount.z, 3);
    }
}

// The group counts are passed on to QRhiCommandBuffer::dispatch(), which does
// not validate them, and a negative count reaches the backend as a very large
// unsigned work group count. Negative counts must therefore be rejected here,
// leaving the previous value in place. Zero is valid and disables the pass.
void tst_QQuick3DComputePass::testDispatchCommandRejectsInvalidGroupCounts()
{
    QQmlEngine engine;

    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; Dispatch { groupCountX: 8; groupCountY: 8; groupCountZ: 8 }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        auto *dispatch = qobject_cast<QQuick3DShaderUtilsCommandDispatch *>(object.data());
        QVERIFY(dispatch != nullptr);

        QSignalSpy xSpy(dispatch, &QQuick3DShaderUtilsCommandDispatch::groupCountXChanged);
        QSignalSpy ySpy(dispatch, &QQuick3DShaderUtilsCommandDispatch::groupCountYChanged);
        QSignalSpy zSpy(dispatch, &QQuick3DShaderUtilsCommandDispatch::groupCountZChanged);

        dispatch->setGroupCountX(-1);
        dispatch->setGroupCountY(-4096);
        dispatch->setGroupCountZ(-1);

        QCOMPARE(dispatch->groupCountX(), 8);
        QCOMPARE(dispatch->groupCountY(), 8);
        QCOMPARE(dispatch->groupCountZ(), 8);
        QCOMPARE(xSpy.size(), 0);
        QCOMPARE(ySpy.size(), 0);
        QCOMPARE(zSpy.size(), 0);

        // A valid count still applies, and reaches the backend command.
        dispatch->setGroupCountX(2);
        QCOMPARE(dispatch->groupCountX(), 2);
        QCOMPARE(xSpy.size(), 1);

        QScopedPointer<QSSGCommand> cloned(dispatch->cloneCommand());
        QVERIFY(cloned != nullptr);
        auto *clonedDispatch = static_cast<QSSGDispatch *>(cloned.data());
        QCOMPARE(clonedDispatch->m_groupCount.x, 2);
        QCOMPARE(clonedDispatch->m_groupCount.y, 8);
        QCOMPARE(clonedDispatch->m_groupCount.z, 8);
    }

    // The same from QML, where a negative binding leaves the default.
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; Dispatch { groupCountX: -1; groupCountY: 0; groupCountZ: 4 }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        auto *dispatch = qobject_cast<QQuick3DShaderUtilsCommandDispatch *>(object.data());
        QVERIFY(dispatch != nullptr);

        QCOMPARE(dispatch->groupCountX(), 1);
        QCOMPARE(dispatch->groupCountY(), 0);
        QCOMPARE(dispatch->groupCountZ(), 4);
    }
}

// ShaderImage and ShaderStorageBuffer properties that are null at the first
// sync have no object to watch. Assigning them later still has to be noticed,
// and since the properties gain an object, the shader has to be processed
// again so that the declarations are generated. The assigned objects have to
// join the pass's scene.
void tst_QQuick3DComputePass::testNullResourcePropertiesAssignedLater()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("null.comp"), computeShader("rgba8"));
    QVERIFY(!shaderUrl.isEmpty());

    QQuick3DSceneManager manager;
    QQmlEngine engine;

    // Declared before the pass, so that the pass releases its references first.
    QScopedPointer<QQuick3DShaderImage> image(createResource<QQuick3DShaderImage>(
            engine, "import QtQuick3D; ShaderImage { width: 4; height: 4; format: ShaderImage.RGBA8 }"));
    QVERIFY(image != nullptr);
    QScopedPointer<QQuick3DShaderStorageBuffer> buffer(createResource<QQuick3DShaderStorageBuffer>(
            engine, "import QtQuick3D; ShaderStorageBuffer { size: 16 }"));
    QVERIFY(buffer != nullptr);

    QScopedPointer<QQuick3DComputePass> pass(createResource<QQuick3DComputePass>(engine, computePassQml));
    QVERIFY(pass != nullptr);
    QQuick3DObjectPrivate::refSceneManager(pass.data(), manager);
    pass->setComputeShader(shaderUrl);

    // Without objects, the declarations in the shader have nothing to pair with.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Unable to find matching buffer or property for")));
    QTest::ignoreMessage(QtWarningMsg, "ComputePass: Shader Image and/or Buffer processing failed in compute shader.");
    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass.data(), nullptr));
    QVERIFY(node != nullptr);
    auto *computeNode = static_cast<QSSGRenderComputePass *>(node.data());
    QVERIFY(computeNode->computeShader.metadata.isEmpty());

    const auto releaseNodes = qScopeGuard([&] {
        releaseNode(image.data());
        releaseNode(buffer.data());
    });
    QVERIFY(createNode(image.data()) != nullptr);
    QVERIFY(createNode(buffer.data()) != nullptr);
    pass->setProperty("myImage", QVariant::fromValue(image.data()));
    pass->setProperty("myBuffer", QVariant::fromValue(buffer.data()));

    QSSGRenderGraphObject *sameNode = QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());
    QCOMPARE(sameNode, node.data());

    const QByteArrayList expected { "buffer", "vec4 values[];", "myBuffer", "readonly",
                                    "image2D", "rgba8", "myImage", "writeonly" };
    QCOMPARE(computeNode->computeShader.metadata, expected);
    QVERIFY(computeNode->computeShader.body.contains("@myImage@"));
    QVERIFY(computeNode->computeShader.body.contains("@myBuffer@"));

    const QSSGBaseTypeProperty *imageProp = findUniform(computeNode->computeShader, "myImage");
    QVERIFY(imageProp != nullptr);
    QCOMPARE(imageProp->value.value<QSSGShaderImage>().image,
             static_cast<QSSGRenderImage *>(QQuick3DObjectPrivate::get(image.data())->spatialNode));
    const QSSGBaseTypeProperty *bufferProp = findUniform(computeNode->computeShader, "myBuffer");
    QVERIFY(bufferProp != nullptr);
    QCOMPARE(bufferProp->value.value<QSSGShaderBuffer>().buffer,
             static_cast<QSSGRenderBuffer *>(QQuick3DObjectPrivate::get(buffer.data())->spatialNode));

    QCOMPARE(QQuick3DObjectPrivate::get(image.data())->sceneManager.data(), &manager);
    QCOMPARE(QQuick3DObjectPrivate::get(buffer.data())->sceneManager.data(), &manager);
}

// Swapping an object for another of the same kind only changes the binding:
// the shader is not processed again, and the scene references move from the
// old object to the new one. Swapping an image for one of another format
// changes the generated declaration, so the shader has to be processed again,
// even when the shader itself changed in an earlier sync. That earlier
// processing failed on the old image's format, and nothing else would retry it.
void tst_QQuick3DComputePass::testResourceSwap()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl rgba8Url = writeShader(dir, QStringLiteral("rgba8.comp"), computeShader("rgba8"));
    const QUrl r32fUrl = writeShader(dir, QStringLiteral("r32f.comp"), computeShader("r32f"));
    QVERIFY(!rgba8Url.isEmpty());
    QVERIFY(!r32fUrl.isEmpty());

    QQuick3DSceneManager manager;
    QQmlEngine engine;

    const QByteArray rgba8Image("import QtQuick3D; ShaderImage { width: 4; height: 4; format: ShaderImage.RGBA8 }");
    const QByteArray r32fImage("import QtQuick3D; ShaderImage { width: 4; height: 4; format: ShaderImage.R32F }");
    const QByteArray storageBuffer("import QtQuick3D; ShaderStorageBuffer { size: 16 }");
    QScopedPointer<QQuick3DShaderImage> imageA(createResource<QQuick3DShaderImage>(engine, rgba8Image));
    QScopedPointer<QQuick3DShaderImage> imageB(createResource<QQuick3DShaderImage>(engine, rgba8Image));
    QScopedPointer<QQuick3DShaderImage> imageC(createResource<QQuick3DShaderImage>(engine, r32fImage));
    QScopedPointer<QQuick3DShaderStorageBuffer> bufferA(createResource<QQuick3DShaderStorageBuffer>(engine, storageBuffer));
    QScopedPointer<QQuick3DShaderStorageBuffer> bufferB(createResource<QQuick3DShaderStorageBuffer>(engine, storageBuffer));
    QVERIFY(imageA && imageB && imageC && bufferA && bufferB);
    const QList<QQuick3DObject *> resources { imageA.data(), imageB.data(), imageC.data(), bufferA.data(), bufferB.data() };
    const auto imageNode = [](QQuick3DShaderImage *image) {
        return static_cast<QSSGRenderImage *>(QQuick3DObjectPrivate::get(image)->spatialNode);
    };
    const auto sceneManagerOf = [](QQuick3DObject *object) {
        return QQuick3DObjectPrivate::get(object)->sceneManager.data();
    };

    QScopedPointer<QQuick3DComputePass> pass(createResource<QQuick3DComputePass>(engine, computePassQml));
    QVERIFY(pass != nullptr);
    QQuick3DObjectPrivate::refSceneManager(pass.data(), manager);
    const auto releaseNodes = qScopeGuard([&] {
        for (QQuick3DObject *object : resources)
            releaseNode(object);
    });
    for (QQuick3DObject *object : resources)
        QVERIFY(createNode(object) != nullptr);

    pass->setComputeShader(rgba8Url);
    pass->setProperty("myImage", QVariant::fromValue(imageA.data()));
    pass->setProperty("myBuffer", QVariant::fromValue(bufferA.data()));

    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass.data(), nullptr));
    QVERIFY(node != nullptr);
    auto *computeNode = static_cast<QSSGRenderComputePass *>(node.data());
    QVERIFY(computeNode->isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty));
    QCOMPARE(findUniform(computeNode->computeShader, "myImage")->value.value<QSSGShaderImage>().image, imageNode(imageA.data()));
    QCOMPARE(sceneManagerOf(imageA.data()), &manager);
    QCOMPARE(sceneManagerOf(bufferA.data()), &manager);

    // Stands in for the renderer having built the shader.
    computeNode->clearDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty);

    // Same kind, same format: a binding change only.
    releaseNode(imageA.data());
    releaseNode(bufferA.data());
    pass->setProperty("myImage", QVariant::fromValue(imageB.data()));
    pass->setProperty("myBuffer", QVariant::fromValue(bufferB.data()));
    QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());

    QVERIFY(!computeNode->isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty));
    QCOMPARE(findUniform(computeNode->computeShader, "myImage")->value.value<QSSGShaderImage>().image, imageNode(imageB.data()));
    QCOMPARE(findUniform(computeNode->computeShader, "myBuffer")->value.value<QSSGShaderBuffer>().buffer,
             static_cast<QSSGRenderBuffer *>(QQuick3DObjectPrivate::get(bufferB.data())->spatialNode));
    QCOMPARE(sceneManagerOf(imageA.data()), nullptr);
    QCOMPARE(sceneManagerOf(bufferA.data()), nullptr);
    QCOMPARE(sceneManagerOf(imageB.data()), &manager);
    QCOMPARE(sceneManagerOf(bufferB.data()), &manager);

    // The shader moves to r32f first, which cannot be processed with the
    // rgba8 image still bound...
    pass->setComputeShader(r32fUrl);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Shader image format doesn't match format in shader")));
    QTest::ignoreMessage(QtWarningMsg, "ComputePass: Shader Image and/or Buffer processing failed in compute shader.");
    QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());
    QVERIFY(!computeNode->isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty));

    // ...and the r32f image follows in a later sync.
    releaseNode(imageB.data());
    pass->setProperty("myImage", QVariant::fromValue(imageC.data()));
    QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());

    QVERIFY2(computeNode->isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty),
             "the shader was not processed again after the image of another format was assigned");
    QVERIFY(computeNode->computeShader.metadata.contains("r32f"));
    const QSSGShaderImage resolved = findUniform(computeNode->computeShader, "myImage")->value.value<QSSGShaderImage>();
    QCOMPARE(resolved.image, imageNode(imageC.data()));
    QCOMPARE(resolved.format, QByteArray("r32f"));
    QCOMPARE(sceneManagerOf(imageB.data()), nullptr);
    QCOMPARE(sceneManagerOf(imageC.data()), &manager);

    // Changing the format of the bound image changes the declaration too.
    computeNode->clearDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty);
    imageC->setFormat(QQuick3DShaderImage::RGBA8);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Shader image format doesn't match format in shader")));
    QTest::ignoreMessage(QtWarningMsg, "ComputePass: Shader Image and/or Buffer processing failed in compute shader.");
    QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());

    // So does changing its shape: the shader declares an image2D.
    imageC->setFormat(QQuick3DShaderImage::R32F);
    QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());
    QVERIFY(computeNode->isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty));
    computeNode->clearDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty);
    imageC->setShape(QQuick3DShaderImage::Image3D);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Shader image type doesn't match type in shader")));
    QTest::ignoreMessage(QtWarningMsg, "ComputePass: Shader Image and/or Buffer processing failed in compute shader.");
    QQuick3DObjectPrivate::updateSpatialNode(pass.data(), node.data());
    QVERIFY(!computeNode->isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty));
}

QTEST_MAIN(tst_QQuick3DComputePass)
#include "tst_qquick3dcomputepass.moc"
