// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#include <QtCore/qvector.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>

class tst_renderer : public QObject
{
    Q_OBJECT

public:
    tst_renderer();
    ~tst_renderer() = default;

private Q_SLOTS:
    void initTestCase();
    void bench_prep();

private:
    QRhi *rhi = nullptr;
    QSSGRef<QSSGRenderContextInterface> renderContext;
    QSharedPointer<QQuick3DSceneManager> sceneManager;

    QString meshPath;
    int modelCount = 0;
    QSSGRenderCamera camera;
    QSSGRenderLayer layer;
};

tst_renderer::tst_renderer()
{
}

void tst_renderer::initTestCase()
{
    sceneManager.reset(new QQuick3DSceneManager);

    rhi = QRhi::create(QRhi::Null, nullptr);
    QRhiCommandBuffer *cb;
    rhi->beginOffscreenFrame(&cb);

    const auto rhiContext = QSSGRef<QSSGRhiContext>(new QSSGRhiContext);
    rhiContext->initialize(rhi);
    rhiContext->setCommandBuffer(cb);

    auto shaderCache = new QSSGShaderCache(rhiContext);
    renderContext = QSSGRef<QSSGRenderContextInterface>(new QSSGRenderContextInterface(rhiContext,
                                                                                       new QSSGBufferManager(rhiContext, shaderCache),
                                                                                       new QSSGResourceManager(rhiContext),
                                                                                       new QSSGRenderer,
                                                                                       new QSSGShaderLibraryManager,
                                                                                       shaderCache,
                                                                                       new QSSGCustomMaterialSystem,
                                                                                       new QSSGProgramGenerator));
    sceneManager->rci = renderContext.data();

    meshPath = qEnvironmentVariable("tst_mesh", "#Cube");

    bool ok = true;
    int n = qEnvironmentVariableIntValue("tst_count", &ok);
    if (!ok)
        n = 22; // 22^3 = 10648 models

    modelCount = n * n * n;

    Q_ASSERT(n > 0 && n <= 100); // Note: Models = count^3

    const float spacing = 20.0f;
    const float offset = -spacing * float(n) * 0.5f;

    layer.activeCamera = &camera;

    renderContext->setWindowDimensions(QSize(800,600));
    const auto viewport = QRect(QPoint(), QSize(800,600));
    renderContext->setViewport(viewport);
    renderContext->setScissorRect(viewport);
    renderContext->setSceneColor(QColor(Qt::black));

    for (int x = 0; x != n; ++x) {
        for (int y = 0; y != n; ++y) {
            for (int z = 0; z != n; ++z) {
                // Set-up model
                QSSGRenderModel *model = new QSSGRenderModel;
                model->meshPath = QSSGRenderPath("#Cube");
                model->scale = QVector3D(0.1f, 0.1f, 0.1f);
                model->position = QVector3D((float(x) + offset) * spacing,
                                            (float(y) + offset) * spacing,
                                            (float(z) + offset) * spacing);

                // Set-up material
                QSSGRenderDefaultMaterial *mat = new QSSGRenderDefaultMaterial;
                mat->color = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
                mat->opacity = (z % 2) ? 1.0f : 0.5f;
                mat->lighting = QSSGRenderDefaultMaterial::MaterialLighting::NoLighting;

                model->materials.push_back(mat);
                layer.addChild(*model);
            }
        }
    }
}

void tst_renderer::bench_prep()
{
    QVERIFY(!layer.children.isEmpty());
    QBENCHMARK {
        renderContext->beginFrame();
        renderContext->prepareLayerForRender(layer);
        renderContext->endFrame();
    }
}

QTEST_APPLESS_MAIN(tst_renderer)

#include "tst_renderer.moc"
