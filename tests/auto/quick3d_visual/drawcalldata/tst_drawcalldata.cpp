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

QTEST_MAIN(tst_DrawCallData)
#include "tst_drawcalldata.moc"
