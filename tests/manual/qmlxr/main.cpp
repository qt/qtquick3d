// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuick3DObject>
#include <QVector3D>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <private/qquick3dxrview_p.h>
#include <private/qquick3dsceneenvironment_p.h>
#include <rhi/qrhi.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);
    QGuiApplication app(argc, argv);

    QCommandLineParser cmdLineParser;
    const QString appDesc = QString::asprintf("qmlxr %s", qVersion());
    cmdLineParser.setApplicationDescription(appDesc);
    app.setApplicationVersion(QLatin1String(QT_VERSION_STR));
    cmdLineParser.addHelpOption();
    cmdLineParser.addPositionalArgument(QLatin1String("file"),
                                        QObject::tr(".qml file with a Node root"),
                                        QObject::tr("file"));

    QCommandLineOption multiviewOption({ "w", "multiview" }, QObject::tr("Enable multiview rendering (if 3D API supports it)"));
    cmdLineParser.addOption(multiviewOption);

    QCommandLineOption shaderDebugOption({ "s", "shaders" }, QObject::tr("Print generated material shaders"));
    cmdLineParser.addOption(shaderDebugOption);

    QCommandLineOption captureOption({ "c", "capture" }, QObject::tr("Take RenderDoc frame captures (qtbase must be built with RenderDoc support)"));
    cmdLineParser.addOption(captureOption);

    QCommandLineOption grabOption({ "b", "grab" }, QObject::tr("Save frames into frame_[index]_[eye].png files"));
    cmdLineParser.addOption(grabOption);

    QCommandLineOption exitOption({ "x", "exit-after" }, QObject::tr("Exit after <num_frames> frames"), QObject::tr("num_frames"));
    cmdLineParser.addOption(exitOption);

    QCommandLineOption debugOption({ "l", "validate" }, QObject::tr("Enable D3D12/Vulkan/OpenXR debug or validation layer, if available. Also enables QRhi leak checking."));
    cmdLineParser.addOption(debugOption);

    QCommandLineOption msaaOption({ "a", "msaa" }, QObject::tr("Request MSAA with <num_samples> samples. <num_samples> is 2 or 4."), QObject::tr("num_samples"));
    cmdLineParser.addOption(msaaOption);

    QCommandLineOption submitDepthOption({ "e", "submit-depth" }, QObject::tr("Forces submitting the depth buffer (XR_KHR_composition_layer_depth), if supported."));
    cmdLineParser.addOption(submitDepthOption);

    QCommandLineOption nullOption({ "n", "null" }, QLatin1String("Use QRhi backend Null"));
    cmdLineParser.addOption(nullOption);
    QCommandLineOption glOption({ "g", "opengl" }, QLatin1String("Use QRhi backend OpenGL (ES)"));
    cmdLineParser.addOption(glOption);
    QCommandLineOption vkOption({ "v", "vulkan" }, QLatin1String("Use QRhi backend Vulkan [default on Android]"));
    cmdLineParser.addOption(vkOption);
    QCommandLineOption d3d11Option({ "d", "d3d11" }, QLatin1String("Use QRhi backend Direct3D 11"));
    cmdLineParser.addOption(d3d11Option);
    QCommandLineOption d3d12Option({ "D", "d3d12" }, QLatin1String("Use QRhi backend Direct3D 12 [default on Windows]"));
    cmdLineParser.addOption(d3d12Option);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Use QRhi backend Metal"));
    cmdLineParser.addOption(mtlOption);

    cmdLineParser.process(app);

    if (cmdLineParser.positionalArguments().isEmpty()) {
        cmdLineParser.showHelp();
        return 0;
    }

    const QString filename = cmdLineParser.positionalArguments().first();

    qputenv("QSG_INFO", "1");
    qputenv("QSG_RHI_PROFILE", "1");

#ifdef Q_OS_WIN
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D12);
#endif
#ifdef Q_OS_ANDROID
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
#endif

    if (cmdLineParser.isSet(nullOption))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Null);
    else if (cmdLineParser.isSet(glOption))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    else if (cmdLineParser.isSet(vkOption))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
    else if (cmdLineParser.isSet(d3d11Option))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
    else if (cmdLineParser.isSet(d3d12Option))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D12);
    else if (cmdLineParser.isSet(mtlOption))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    if (cmdLineParser.isSet(multiviewOption))
        qputenv("QT_QUICK3D_XR_MULTIVIEW", "1");

    if (cmdLineParser.isSet(shaderDebugOption))
        qputenv("QT_RHI_SHADER_DEBUG", "1");

    if (cmdLineParser.isSet(captureOption)) {
        qputenv("QT_QUICK3D_SHADER_DEBUG_INFO", "1");
        qputenv("QT_QUICK3D_XR_FRAME_CAPTURE", "1");
    }

    if (cmdLineParser.isSet(debugOption)) {
        qputenv("QSG_RHI_DEBUG_LAYER", "1");
        qputenv("QT_RHI_LEAK_CHECK", "1");
    }

    if (cmdLineParser.isSet(submitDepthOption))
        qputenv("QT_QUICK3D_XR_SUBMIT_DEPTH", "1");

    QCoreApplication::setApplicationName("Qt XR Runner");
    QCoreApplication::setOrganizationName("The Qt Company");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    const QList<QObject *> rootObjects = engine.rootObjects();
    std::unique_ptr<QQmlComponent> component;
    int currentSingleViewEye = 0;
    int currentFrame = 1;
    int exitAfter = 0;
    if (cmdLineParser.isSet(exitOption))
        exitAfter = qMax(0, cmdLineParser.value(exitOption).toInt());

    if (!rootObjects.isEmpty()) {
        QQuick3DXrView *xrView = qobject_cast<QQuick3DXrView *>(rootObjects.first());

        component.reset(new QQmlComponent(&engine, QUrl::fromLocalFile(filename)));
        if (component->isError()) {
            for (const QQmlError &error : component->errors()) {
                qWarning() << error.url() << error.line() << error;
                return 1;
            }
        }
        QObject *subRoot = component->create();
        if (component->isError()) {
            for (const QQmlError &error : component->errors()) {
                qWarning() << error.url() << error.line() << error;
                return 1;
            }
        }

        if (QQuick3DObject *obj = qobject_cast<QQuick3DObject *>(subRoot)) {
            obj->setParentItem(xrView);
            QObject *origin = xrView->findChild<QObject *>("xrorigin");
            QVariant posProp = obj->property("qmlxr_originPosition");
            if (!posProp.isNull()) {
                QVector3D camPos = posProp.value<QVector3D>();
                qDebug() << "Setting origin position to" << camPos;
                origin->setProperty("position", camPos);
            }
            QVariant rotProp = obj->property("qmlxr_originRotation");
            if (!rotProp.isNull()) {
                QVector3D camRot = rotProp.value<QVector3D>();
                qDebug() << "Setting origin rotation to" << camRot;
                origin->setProperty("eulerRotation", camRot);
            }
            QVariant envProp = obj->property("qmlxr_environment");
            if (!envProp.isNull()) {
                QQuick3DSceneEnvironment *env = envProp.value<QQuick3DSceneEnvironment *>();
                qDebug() << "Setting SceneEnvironment to" << env;
                xrView->setEnvironment(env);
            }
        } else {
            qWarning() << subRoot << "is not a QQuick3DObject";
            return 1;
        }

        if (cmdLineParser.isSet(msaaOption)) {
            const int samples = qBound(1, cmdLineParser.value(msaaOption).toInt(), 8);
            if (samples > 1) {
                QQuick3DSceneEnvironment *sceneEnv = xrView->environment();
                qDebug() << "Requesting MSAA with sample count" << samples << "on" << sceneEnv;
                sceneEnv->setAntialiasingMode(QQuick3DSceneEnvironment::MSAA);
                if (samples == 2)
                    sceneEnv->setAntialiasingQuality(QQuick3DSceneEnvironment::Medium);
                else if (samples == 4)
                    sceneEnv->setAntialiasingQuality(QQuick3DSceneEnvironment::High);
                else
                    sceneEnv->setAntialiasingQuality(QQuick3DSceneEnvironment::VeryHigh);
            }
        }

        QObject::connect(xrView, &QQuick3DXrView::frameReady, xrView, Qt::DirectConnection, [&]() {
            QRhiTexture *colorBuffer = nullptr;
            if (xrView->view() && xwView->view()->window()) {
                QQuickWindow *quickWindow = xrView->view()->window();
                QRhiRenderTarget *rt = QQuickWindowPrivate::get(quickWindow)->activeCustomRhiRenderTarget();
                if (rt && rt->resourceType() == QRhiResource::TextureRenderTarget)
                    colorBuffer = static_cast<QRhiTextureRenderTarget *>(rt)->description().colorAttachmentAt(0)->texture();
            }

            const int viewCount = qMax(1, colorBuffer->arraySize());

            if (cmdLineParser.isSet(grabOption)) {
                QRhi *rhi = colorBuffer->rhi();
                QRhiCommandBuffer *cb = nullptr;
                if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
                    return;
                QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();
                QVarLengthArray<QImage, 2> images(viewCount);
                QVarLengthArray<QRhiReadbackResult, 2> results(viewCount);
                for (int layer = 0; layer < viewCount; ++layer) {
                    results[layer].completed = [&results, &images, rhi, layer] {
                        const uchar *p = reinterpret_cast<const uchar *>(results[layer].data.constData());
                        const QImage::Format imageFormat = results[layer].format == QRhiTexture::BGRA8 ? QImage::Format_ARGB32 : QImage::Format_RGBA8888;
                        const QImage img(p, results[layer].pixelSize.width(), results[layer].pixelSize.height(), imageFormat);
                        // the order of the callbacks is undefined, hence must rely on the layer index
                        if (rhi->isYUpInFramebuffer())
                            images[layer] = img.mirrored();
                        else
                            images[layer] = img.copy();
                    };
                    QRhiReadbackDescription readbackDesc(colorBuffer);
                    readbackDesc.setLayer(layer);
                    resourceUpdates->readBackTexture(readbackDesc, &results[layer]);
                }
                cb->resourceUpdate(resourceUpdates);
                rhi->endOffscreenFrame();
                if (images.count() >= 2) {
                    // multiview, this is the complete frame (2D texture array)
                    for (int i = 0; i < images.count(); ++i)
                        images[i].save(QString::asprintf("frame_%d_%d.png", currentFrame, i));
                } else if (images.count() == 1) {
                    // this is either the left or right eye content (2D texture)
                    images[0].save(QString::asprintf("frame_%d_%d.png", currentFrame, currentSingleViewEye));
                }
            }

            if (viewCount >= 2) {
                currentFrame += 1;
            } else {
                currentSingleViewEye = (currentSingleViewEye + 1) % 2;
                if (currentSingleViewEye == 0)
                    currentFrame += 1;
            }

            if (exitAfter > 0 && currentFrame > exitAfter)
                QCoreApplication::exit();
        }, Qt::DirectConnection);
    }

    return app.exec();
}
