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

class tst_MaterialSwap : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void swapDoesNotGrowDrawCallData();
    void skyMaterialSwapDoesNotGrowDrawCallData();

private:
    void runSwapTest(const QString &qmlFile);
#if QT_CONFIG(vulkan)
    QVulkanInstance vulkanInstance;
#endif
};

void tst_MaterialSwap::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;

#if QT_CONFIG(vulkan)
    vulkanInstance.setLayers({ "VK_LAYER_LUNARG_standard_validation" });
    vulkanInstance.create(); // may fail, which is fine if Vulkan is not used in the first place
#endif
}

void tst_MaterialSwap::swapDoesNotGrowDrawCallData()
{
    runSwapTest(QString::fromLatin1("materialSwap.qml"));
}

void tst_MaterialSwap::skyMaterialSwapDoesNotGrowDrawCallData()
{
    runSwapTest(QString::fromLatin1("skyMaterialSwap.qml"));
}

void tst_MaterialSwap::runSwapTest(const QString &qmlFile)
{
    QQuick3DTestOffscreenRenderer renderer;
    void *vulkanInstancePtr = nullptr;
#if QT_CONFIG(vulkan)
    vulkanInstancePtr = &vulkanInstance;
#endif
    QVERIFY(renderer.init(testFileUrl(qmlFile), vulkanInstancePtr));

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

    const auto swapAndRenderFrame = [&](bool swapped) {
        renderer.rootItem->setProperty("swapped", swapped);
        renderer.renderNextFrame();
    };

    // Warm up so both materials have been through creation, destruction and
    // re-creation at least once; the sizes after this represent the expected
    // steady state (entries for the currently assigned material, plus at most
    // one swap's worth of not-yet-cleaned-up entries).
    for (int i = 1; i <= 4; ++i)
        swapAndRenderFrame(i % 2 != 0);

    const qsizetype baselineDrawCallData = rhiCtxD->m_drawCallData.size();
    const qsizetype baselineSrbCache = rhiCtxD->m_srbCache.size();
    QVERIFY(baselineDrawCallData > 0);

    constexpr int SwapCount = 20;
    for (int i = 1; i <= SwapCount; ++i)
        swapAndRenderFrame(i % 2 == 0);

    // Without the cleanup of material-keyed draw call data the caches grow by
    // at least one entry per swap; with it the sizes stay at the steady state.
    // Allow a little slack so the test does not depend on the exact frame at
    // which the deferred cleanup of the previous material runs.
    constexpr qsizetype Slack = 4;
    const qsizetype finalDrawCallData = rhiCtxD->m_drawCallData.size();
    const qsizetype finalSrbCache = rhiCtxD->m_srbCache.size();
    QVERIFY2(finalDrawCallData <= baselineDrawCallData + Slack,
             qPrintable(QString::fromLatin1("m_drawCallData grew from %1 to %2 entries over %3 material swaps")
                                .arg(baselineDrawCallData).arg(finalDrawCallData).arg(SwapCount)));
    QVERIFY2(finalSrbCache <= baselineSrbCache + Slack,
             qPrintable(QString::fromLatin1("m_srbCache grew from %1 to %2 entries over %3 material swaps")
                                .arg(baselineSrbCache).arg(finalSrbCache).arg(SwapCount)));
}

QTEST_MAIN(tst_MaterialSwap)
#include "tst_materialswap.moc"
