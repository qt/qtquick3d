// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtTest>

#include <QtCore/qvector.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
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
    std::shared_ptr<QSSGRenderContextInterface> renderContext;
    QSharedPointer<QQuick3DSceneManager> sceneManager;
    QQuick3DWindowAttachment *wa = nullptr;

    QString meshPath;
    int modelCount = 0;
    QSSGRenderCamera camera{ QSSGRenderCamera::Type::OrthographicCamera };
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

    std::unique_ptr<QSSGRhiContext> rhiContext = std::make_unique<QSSGRhiContext>();
    rhiContext->initialize(rhi);
    rhiContext->setCommandBuffer(cb);

    renderContext = std::make_shared<QSSGRenderContextInterface>(std::make_unique<QSSGBufferManager>(),
                                                                 std::make_unique<QSSGRenderer>(),
                                                                 std::make_shared<QSSGShaderLibraryManager>(),
                                                                 std::make_unique<QSSGShaderCache>(*rhiContext),
                                                                 std::make_unique<QSSGCustomMaterialSystem>(),
                                                                 std::make_unique<QSSGProgramGenerator>(),
                                                                 std::move(rhiContext));

    wa = new QQuick3DWindowAttachment(nullptr);
    wa->setRci(renderContext);
    sceneManager->wattached = wa;

    meshPath = qEnvironmentVariable("tst_mesh", "#Cube");

    bool ok = true;
    int n = qEnvironmentVariableIntValue("tst_count", &ok);
    if (!ok)
        n = 22; // 22^3 = 10648 models

    modelCount = n * n * n;

    Q_ASSERT(n > 0 && n <= 100); // Note: Models = count^3

    const float spacing = 20.0f;
    const float offset = -spacing * float(n) * 0.5f;

    layer.explicitCamera = &camera;

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
                model->localTransform.translate(QVector3D((float(x) + offset) * spacing,
                                                          (float(y) + offset) * spacing,
                                                          (float(z) + offset) * spacing));

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
        renderContext->beginFrame(&layer);
        renderContext->prepareLayerForRender(layer);
        renderContext->endFrame(&layer);
    }
}

QTEST_APPLESS_MAIN(tst_renderer)

#include "tst_renderer.moc"
