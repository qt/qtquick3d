// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <private/qquick3dviewport_p.h>
#include <ssg/qssgrendercontextcore.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>

#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#endif

#include "../shared/util.h"

class tst_DrawCallData : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void viewChurnReleasesDrawCallData();
    void viewChurnReleasesShaderResourceBindings();
    void resizeDoesNotGrowPipelineCache();
    void resizeDoesNotGrowSrbCacheForQuadRenderer();
    void computePassChurnReleasesDrawCallData_data();
    void computePassChurnReleasesDrawCallData();

private:
#if QT_CONFIG(vulkan)
    QVulkanInstance vulkanInstance;
#endif
};

void tst_DrawCallData::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;

#if QT_CONFIG(vulkan)
    vulkanInstance.setLayers({ "VK_LAYER_LUNARG_standard_validation" });
    vulkanInstance.create(); // may fail, which is fine if Vulkan is not used in the first place
#endif
}

// Draw call data for rendered content is keyed on the pass objects of the
// layer it was rendered by. When a View3D is destroyed while its content
// lives on, e.g. a shared import scene, the entries keyed on the destroyed
// layer's passes must be released; otherwise every destroyed view leaks its
// draw call data (uniform buffers and cached srbs) for as long as the content
// is alive.
void tst_DrawCallData::viewChurnReleasesDrawCallData()
{
    QQuick3DTestOffscreenRenderer renderer;
    void *vulkanInstancePtr = nullptr;
#if QT_CONFIG(vulkan)
    vulkanInstancePtr = &vulkanInstance;
#endif
    QVERIFY(renderer.init(testFileUrl(QString::fromLatin1("viewChurn.qml")), vulkanInstancePtr));

#ifdef Q_OS_MACOS
    if (renderer.quickWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL)
        QSKIP("Skipping test due to software OpenGL renderer problems on macOS");
#endif

    renderer.renderNextFrame();

    const auto &context = QQuick3DSceneManager::getOrSetWindowAttachment(*renderer.quickWindow)->rci();
    QVERIFY(context);

    const auto &rhiContext = context->rhiContext();
    QVERIFY(rhiContext);
    const QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiContext.get());

    const auto setViewLoadedAndRenderFrames = [&](bool loaded) {
        renderer.rootItem->setProperty("loadView", loaded);
        // The view is destroyed on unload during the first sync; the layer and
        // its render data are queued for cleanup and deleted during the next
        // one, so render a few frames to let the deferred cleanup run.
        for (int frame = 0; frame < 3; ++frame)
            renderer.renderNextFrame();
    };

    const qsizetype loadedSize = rhiCtxD->m_drawCallData.size();
    QVERIFY(loadedSize > 0);

    setViewLoadedAndRenderFrames(false);
    const qsizetype unloadedSize = rhiCtxD->m_drawCallData.size();

    constexpr int ChurnCount = 5;
    for (int i = 0; i < ChurnCount; ++i) {
        setViewLoadedAndRenderFrames(true);
        setViewLoadedAndRenderFrames(false);
    }

    // Without the per-pass cleanup each cycle leaks the draw call data of the
    // shared content; with it the size returns to the unloaded baseline.
    const qsizetype finalUnloadedSize = rhiCtxD->m_drawCallData.size();
    QVERIFY2(finalUnloadedSize <= unloadedSize,
             qPrintable(QString::fromLatin1("m_drawCallData grew from %1 to %2 entries over %3 view load/unload cycles")
                                .arg(unloadedSize).arg(finalUnloadedSize).arg(ChurnCount)));

    // And loading again must not accumulate on top of leaked entries either.
    setViewLoadedAndRenderFrames(true);
    const qsizetype reloadedSize = rhiCtxD->m_drawCallData.size();
    QVERIFY2(reloadedSize <= loadedSize,
             qPrintable(QString::fromLatin1("m_drawCallData grew from %1 to %2 entries after view churn")
                                .arg(loadedSize).arg(reloadedSize)));
}

// The depth pre-pass, shadow, normal and user passes build their srbs around the
// uniform buffer of their draw call data. Unless the srb is owned by the draw call
// data, releasing the draw call data with a destroyed view leaves the srb in the
// cache, still referencing the deleted uniform buffer, so the cache grows with
// every destroyed view (or window) for as long as the content is alive.
void tst_DrawCallData::viewChurnReleasesShaderResourceBindings()
{
    QQuick3DTestOffscreenRenderer renderer;
    void *vulkanInstancePtr = nullptr;
#if QT_CONFIG(vulkan)
    vulkanInstancePtr = &vulkanInstance;
#endif
    QVERIFY(renderer.init(testFileUrl(QString::fromLatin1("srbChurn.qml")), vulkanInstancePtr));

#ifdef Q_OS_MACOS
    if (renderer.quickWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL)
        QSKIP("Skipping test due to software OpenGL renderer problems on macOS");
#endif

    renderer.renderNextFrame();

    const auto &context = QQuick3DSceneManager::getOrSetWindowAttachment(*renderer.quickWindow)->rci();
    QVERIFY(context);

    const auto &rhiContext = context->rhiContext();
    QVERIFY(rhiContext);
    const QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiContext.get());

    const auto setViewLoadedAndRenderFrames = [&](bool loaded) {
        renderer.rootItem->setProperty("loadView", loaded);
        // See viewChurnReleasesDrawCallData() for why more than one frame is needed.
        for (int frame = 0; frame < 3; ++frame)
            renderer.renderNextFrame();
    };

    const qsizetype loadedSize = rhiCtxD->m_srbCache.size();
    QVERIFY(loadedSize > 0);

    setViewLoadedAndRenderFrames(false);
    const qsizetype unloadedSize = rhiCtxD->m_srbCache.size();

    constexpr int ChurnCount = 5;
    for (int i = 0; i < ChurnCount; ++i) {
        setViewLoadedAndRenderFrames(true);
        setViewLoadedAndRenderFrames(false);
    }

    const qsizetype finalUnloadedSize = rhiCtxD->m_srbCache.size();
    QVERIFY2(finalUnloadedSize <= unloadedSize,
             qPrintable(QString::fromLatin1("m_srbCache grew from %1 to %2 entries over %3 view load/unload cycles")
                                .arg(unloadedSize).arg(finalUnloadedSize).arg(ChurnCount)));

    setViewLoadedAndRenderFrames(true);
    const qsizetype reloadedSize = rhiCtxD->m_srbCache.size();
    QVERIFY2(reloadedSize <= loadedSize,
             qPrintable(QString::fromLatin1("m_srbCache grew from %1 to %2 entries after view churn")
                                .arg(loadedSize).arg(reloadedSize)));
}

// The viewport and scissor rects are dynamic state and must not take part in the
// graphics pipeline cache key. When they did, every window resize created a new set
// of pipelines (a full program link on OpenGL) that stayed in the cache forever.
void tst_DrawCallData::resizeDoesNotGrowPipelineCache()
{
    QQuick3DTestOffscreenRenderer renderer;
    void *vulkanInstancePtr = nullptr;
#if QT_CONFIG(vulkan)
    vulkanInstancePtr = &vulkanInstance;
#endif
    QVERIFY(renderer.init(testFileUrl(QString::fromLatin1("viewChurn.qml")), vulkanInstancePtr));

#ifdef Q_OS_MACOS
    if (renderer.quickWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL)
        QSKIP("Skipping test due to software OpenGL renderer problems on macOS");
#endif

    renderer.renderNextFrame();

    const auto &context = QQuick3DSceneManager::getOrSetWindowAttachment(*renderer.quickWindow)->rci();
    QVERIFY(context);

    const auto &rhiContext = context->rhiContext();
    QVERIFY(rhiContext);
    const QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiContext.get());

    const qsizetype baselinePipelines = rhiCtxD->m_pipelines.size();
    QVERIFY(baselinePipelines > 0);

    constexpr int ResizeCount = 20;
    for (int i = 0; i < ResizeCount; ++i) {
        QVERIFY(renderer.resize(QSize(500 + i * 7, 400 + i * 5)));
        renderer.renderNextFrame();
    }

    const qsizetype finalPipelines = rhiCtxD->m_pipelines.size();
    QVERIFY2(finalPipelines <= baselinePipelines,
             qPrintable(QString::fromLatin1("m_pipelines grew from %1 to %2 entries over %3 resizes")
                                .arg(baselinePipelines).arg(finalPipelines).arg(ResizeCount)));
}

// The SimpleQuadRenderer binds a texture to output, which is recreated when the view is resized.
// The srb built for the previous texture must therefore be released then, otherwise every resize
// leaves an srb in the cache that references a deleted texture.
void tst_DrawCallData::resizeDoesNotGrowSrbCacheForQuadRenderer()
{
    QQuick3DTestOffscreenRenderer renderer;
    void *vulkanInstancePtr = nullptr;
#if QT_CONFIG(vulkan)
    vulkanInstancePtr = &vulkanInstance;
#endif
    QVERIFY(renderer.init(testFileUrl(QString::fromLatin1("quadResize.qml")), vulkanInstancePtr));

#ifdef Q_OS_MACOS
    if (renderer.quickWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL)
        QSKIP("Skipping test due to software OpenGL renderer problems on macOS");
#endif

    renderer.renderNextFrame();
    renderer.renderNextFrame();

    const auto &context = QQuick3DSceneManager::getOrSetWindowAttachment(*renderer.quickWindow)->rci();
    QVERIFY(context);

    const auto &rhiContext = context->rhiContext();
    QVERIFY(rhiContext);
    const QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiContext.get());

    const qsizetype baselineSrbs = rhiCtxD->m_srbCache.size();
    QVERIFY(baselineSrbs > 0);

    constexpr int ResizeCount = 10;
    for (int i = 0; i < ResizeCount; ++i) {
        QVERIFY(renderer.resize(QSize(500 + i * 7, 400 + i * 5)));
        renderer.renderNextFrame();
        renderer.renderNextFrame();
    }

    const qsizetype finalSrbs = rhiCtxD->m_srbCache.size();
    QVERIFY2(finalSrbs <= baselineSrbs,
             qPrintable(QString::fromLatin1("m_srbCache grew from %1 to %2 entries over %3 resizes")
                                .arg(baselineSrbs).arg(finalSrbs).arg(ResizeCount)));
}

void tst_DrawCallData::computePassChurnReleasesDrawCallData_data()
{
    QTest::addColumn<QByteArray>("loadProperty");

    // Destroying the ComputePass alone releases its draw call data through the
    // pass node; destroying the whole View3D also takes the layer with it.
    QTest::newRow("pass") << QByteArrayLiteral("loadPass");
    QTest::newRow("view") << QByteArrayLiteral("loadView");
}

// A ComputePass keeps its uniform buffer and srb in draw call data keyed on the
// layer's user pass object and on the pass node itself. Both have to be released
// when the pass goes away, whether on its own or together with its View3D;
// otherwise every destroyed pass leaves a uniform buffer and an srb behind.
void tst_DrawCallData::computePassChurnReleasesDrawCallData()
{
    QFETCH(QByteArray, loadProperty);

    QQuick3DTestOffscreenRenderer renderer;
    void *vulkanInstancePtr = nullptr;
#if QT_CONFIG(vulkan)
    vulkanInstancePtr = &vulkanInstance;
#endif
    QVERIFY(renderer.init(testFileUrl(QString::fromLatin1("computeChurn.qml")), vulkanInstancePtr));

#ifdef Q_OS_MACOS
    if (renderer.quickWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL)
        QSKIP("Skipping test due to software OpenGL renderer problems on macOS");
#endif
    if (!renderer.rhi->isFeatureSupported(QRhi::Compute))
        QSKIP("Compute shaders are not supported by this QRhi backend");

    renderer.renderNextFrame();

    const auto &context = QQuick3DSceneManager::getOrSetWindowAttachment(*renderer.quickWindow)->rci();
    QVERIFY(context);

    const auto &rhiContext = context->rhiContext();
    QVERIFY(rhiContext);
    const QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiContext.get());

    int frameCounter = 0;
    const auto setLoadedAndRenderFrames = [&](bool loaded) {
        renderer.rootItem->setProperty(loadProperty.constData(), loaded);
        // Loader3D destroys the unloaded ComputePass with deleteLater(), which
        // processEvents() in renderNextFrame() does not handle.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        // See viewChurnReleasesDrawCallData() for why more than one frame is
        // needed. The frame counter keeps the other View3D syncing, which is
        // what releases the resources of a destroyed one.
        for (int frame = 0; frame < 3; ++frame) {
            renderer.rootItem->setProperty("frameCounter", ++frameCounter);
            renderer.renderNextFrame();
        }
    };

    setLoadedAndRenderFrames(false);
    const qsizetype unloadedDcd = rhiCtxD->m_drawCallData.size();
    const qsizetype unloadedSrbs = rhiCtxD->m_srbCache.size();

    // Make sure the pass really ran, otherwise there is nothing to release and
    // the checks below would pass vacuously.
    setLoadedAndRenderFrames(true);
    QVERIFY2(rhiCtxD->m_drawCallData.size() > unloadedDcd, "The compute pass created no draw call data");
    QVERIFY2(rhiCtxD->m_srbCache.size() > unloadedSrbs, "The compute pass created no srb");

    constexpr int ChurnCount = 5;
    for (int i = 0; i < ChurnCount; ++i) {
        setLoadedAndRenderFrames(false);
        const qsizetype dcd = rhiCtxD->m_drawCallData.size();
        const qsizetype srbs = rhiCtxD->m_srbCache.size();
        QVERIFY2(dcd <= unloadedDcd,
                 qPrintable(QString::fromLatin1("m_drawCallData has %1 entries after unload %2, expected at most %3")
                                    .arg(dcd).arg(i + 1).arg(unloadedDcd)));
        QVERIFY2(srbs <= unloadedSrbs,
                 qPrintable(QString::fromLatin1("m_srbCache has %1 entries after unload %2, expected at most %3")
                                    .arg(srbs).arg(i + 1).arg(unloadedSrbs)));
        setLoadedAndRenderFrames(true);
    }
}

QTEST_MAIN(tst_DrawCallData)
#include "tst_drawcalldata.moc"
