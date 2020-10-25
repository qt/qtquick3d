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

#include "genshaders.h"

#include <QtCore/qdir.h>

#include <QtGui/private/qrhinull_p_p.h>

#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dscenerenderer_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>

// Lights
#include <QtQuick3D/private/qquick3dspotlight_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>

#include <QtQuick3DUtils/private/qqsbcollection_p.h>

#include <private/qssgrenderer_p.h>
#include <private/qssgrendererimpllayerrenderdata_p.h>

#include <QtShaderTools/private/qshaderbaker_p.h>

static void initBaker(QShaderBaker *baker, QRhi::Implementation target)
{
    Q_UNUSED(target);
    QVector<QShaderBaker::GeneratedShader> outputs;
    // TODO: For simplicity we're just going to add all off these for now.
    outputs.append({ QShader::SpirvShader, QShaderVersion(100) }); // Vulkan 1.0
    outputs.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
    outputs.append({ QShader::MslShader, QShaderVersion(12) }); // Metal 1.2
    outputs.append({ QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs) }); // GLES 3.0+
    outputs.append({ QShader::GlslShader, QShaderVersion(130) }); // OpenGL 3.0+

    baker->setGeneratedShaders(outputs);
    baker->setGeneratedShaderVariants({ QShader::StandardShader });
}

static QQsbShaderFeatureSet toQsbShaderFeatureSet(const ShaderFeatureSetList &featureSet)
{
    QQsbShaderFeatureSet ret;
    for (const auto &f : featureSet)
        ret.insert(f.name, f.enabled);
    return ret;
}

GenShaders::GenShaders(const QString &sourceDir)
{
    sceneManager = new QQuick3DSceneManager;

    rhi = QRhi::create(QRhi::Null, nullptr);
    QRhiCommandBuffer *cb;
    rhi->beginOffscreenFrame(&cb);

    const auto rhiContext = QSSGRef<QSSGRhiContext>(new QSSGRhiContext);
    rhiContext->initialize(rhi);
    rhiContext->setCommandBuffer(cb);

    auto inputStreamFactory = new QSSGInputStreamFactory;
    auto shaderCache = new QSSGShaderCache(rhiContext, inputStreamFactory, &initBaker);
    renderContext = QSSGRef<QSSGRenderContextInterface>(new QSSGRenderContextInterface(rhiContext,
                                                                                       inputStreamFactory,
                                                                                       new QSSGBufferManager(rhiContext, shaderCache, inputStreamFactory),
                                                                                       new QSSGResourceManager(rhiContext),
                                                                                       new QSSGRenderer,
                                                                                       new QSSGShaderLibraryManager(inputStreamFactory),
                                                                                       shaderCache,
                                                                                       new QSSGCustomMaterialSystem,
                                                                                       new QSSGProgramGenerator,
                                                                                       sourceDir));
    sceneManager->rci = renderContext.data();
}

GenShaders::~GenShaders() = default;

bool GenShaders::process(const MaterialParser::SceneData &sceneData,
                         QVector<QString> &qsbcFiles,
                         const QDir &outDir,
                         bool generateMultipleLights,
                         bool dryRun)
{
    Q_UNUSED(generateMultipleLights);

    const QString resourceFolderRelative = QSSGShaderCache::resourceFolder().mid(2);
    if (!dryRun && !outDir.exists(resourceFolderRelative)) {
        if (!outDir.mkpath(resourceFolderRelative)) {
            qDebug("Unable to create folder: %s", qPrintable(outDir.path() + QDir::separator() + resourceFolderRelative));
            return false;
        }
    }

    const QString outputFolder = outDir.canonicalPath() + QDir::separator() + resourceFolderRelative;

    QSSGRenderLayer layer;
    renderContext->setViewport(QRect(QPoint(), QSize(888,666)));
    const auto &renderer = renderContext->renderer();
    QSSGLayerRenderData layerData(layer, renderer);

    const auto &shaderLibraryManager = renderContext->shaderLibraryManager();
    const auto &shaderCache = renderContext->shaderCache();
    const auto &shaderProgramGenerator = renderContext->shaderProgramGenerator();

    bool aaIsDirty = false;
    bool temporalIsDirty = false;
    float ssaaMultiplier = 1.5f;

    QQuick3DViewport *view3D = sceneData.viewport;
    Q_ASSERT(view3D);

    QVector<QSSGRenderGraphObject *> nodes;

    if (!view3D->camera()) {
        auto camera = new QQuick3DPerspectiveCamera();
        auto node = QQuick3DObjectPrivate::updateSpatialNode(camera, nullptr);
        QQuick3DObjectPrivate::get(camera)->spatialNode = node;
        nodes.append(node);
        view3D->setCamera(camera);
    }

    // Realize resources
    // Textures
    const auto &textures = sceneData.textures;
    for (const auto &tex : textures) {
        auto node = QQuick3DObjectPrivate::updateSpatialNode(tex, nullptr);
        auto obj = QQuick3DObjectPrivate::get(tex);
        obj->spatialNode = node;
        nodes.append(node);
    }

    // Free Materials (see also the model section)
    const auto &materials = sceneData.materials;
    for (const auto &mat : materials) {
        auto node = QQuick3DObjectPrivate::updateSpatialNode(mat.ptr, nullptr);
        QQuick3DObjectPrivate::get(mat.ptr)->spatialNode = node;
        nodes.append(node);
    }

    bool shadowCubePass = false;
    bool shadowMapPass = false;

    // Lights
    const auto &lights = sceneData.lights;
    for (const auto &light : lights) {
        auto node = QQuick3DObjectPrivate::updateSpatialNode(light.ptr, nullptr);
        nodes.append(node);
        layer.addChild(static_cast<QSSGRenderNode &>(*node));
        if (light.ptr->castsShadow()) {
            if (light.type == TypeInfo::PointLight)
                shadowCubePass |= true;
            else
                shadowMapPass |= true;
        }
    }

    // NOTE: Model.castsShadows; Model.receivesShadows; variants needs to be added for runtime support
    const auto &models = sceneData.models;
    for (const auto &model : models) {
        auto materialList = model->materials();
        for (int i = 0; i != materialList.count(&materialList); ++i) {
            auto mat = materialList.at(&materialList, i);
            QQuick3DObjectPrivate::get(mat)->sceneManager = sceneManager;
            auto node = QQuick3DObjectPrivate::updateSpatialNode(mat, nullptr);
            QQuick3DObjectPrivate::get(mat)->spatialNode = node;
            nodes.append(node);
        }
        auto node = QQuick3DObjectPrivate::updateSpatialNode(model, nullptr);
        QQuick3DObjectPrivate::get(model)->spatialNode = node;
        nodes.append(node);
    }

    QQuick3DRenderLayerHelpers::updateLayerNodeHelper(*view3D, layer, aaIsDirty, temporalIsDirty, ssaaMultiplier);

    const QString outCollectionFile = outputFolder + QString::fromLatin1(QSSGShaderCache::shaderCollectionFile());
    QQsbCollection qsbc(outCollectionFile);
    if (!qsbc.map(dryRun ? QQsbCollection::Read : QQsbCollection::Write))
        return false;

    QByteArray shaderString;
    const auto generateShaderForModel = [&](QSSGRenderModel &model) {
        layerData.resetForFrame();
        layer.addChild(model);
        layerData.prepareForRender(QSize(888, 666));

        const auto &features = layerData.getShaderFeatureSet();

        auto &materialPropertis = layerData.renderer->defaultMaterialShaderKeyProperties();

        QSSGRenderableObject *renderable = nullptr;
        if (!layerData.opaqueObjects.isEmpty())
            renderable = layerData.opaqueObjects.at(0).obj;
        else if (!layerData.transparentObjects.isEmpty())
            renderable = layerData.transparentObjects.at(0).obj;

        auto generateShader = [&](const ShaderFeatureSetList &features) {
            if (renderable->renderableFlags.testFlag(QSSGRenderableObjectFlag::DefaultMaterialMeshSubset)) {
                auto shaderPipeline = QSSGRenderer::generateRhiShaderPipelineImpl(*static_cast<QSSGSubsetRenderable *>(renderable), shaderLibraryManager, shaderCache, shaderProgramGenerator, materialPropertis, features, shaderString);
                if (!shaderPipeline.isNull()) {
                    const size_t hkey = QSSGShaderCacheKey::generateHashCode(shaderString, features);
                    const auto vertexStage = shaderPipeline->vertexStage();
                    const auto fragmentStage = shaderPipeline->fragmentStage();
                    if (vertexStage && fragmentStage)
                        qsbc.addQsbEntry(shaderString, toQsbShaderFeatureSet(features), vertexStage->shader(), fragmentStage->shader(), hkey);
                }
            } else if (renderable->renderableFlags.testFlag(QSSGRenderableObjectFlag::CustomMaterialMeshSubset)) {
                Q_ASSERT(layerData.camera);
                QSSGCustomMaterialRenderable &cmr(static_cast<QSSGCustomMaterialRenderable &>(*renderable));
                const auto &rhiContext = renderContext->rhiContext();
                const auto pipelineState = rhiContext->graphicsPipelineState(&layerData);
                const auto &cms = renderContext->customMaterialSystem();
                auto shaderPipeline = cms->shadersForCustomMaterial(pipelineState,
                                                                    cmr.material,
                                                                    cmr,
                                                                    features);

                if (shaderPipeline) {
                    shaderString = cmr.material.m_shaderPathKey;
                    const size_t hkey = QSSGShaderCacheKey::generateHashCode(shaderString, features);
                    const auto vertexStage = shaderPipeline->vertexStage();
                    const auto fragmentStage = shaderPipeline->fragmentStage();
                    if (vertexStage && fragmentStage)
                        qsbc.addQsbEntry(shaderString, toQsbShaderFeatureSet(features), vertexStage->shader(), fragmentStage->shader(), hkey);
                }
            }
        };

        if (renderable) {
            generateShader(features);

            ShaderFeatureSetList depthPassFeatures;
            depthPassFeatures.append({ QSSGShaderDefines::DepthPass, true });
            generateShader(depthPassFeatures);

            if (shadowCubePass) {
                ShaderFeatureSetList shadowPassFeatures;
                shadowPassFeatures.append({ QSSGShaderDefines::CubeShadowPass, true });
                generateShader(shadowPassFeatures);
            }

            if (shadowMapPass) {
                ShaderFeatureSetList shadowPassFeatures;
                shadowPassFeatures.append({ QSSGShaderDefines::OrthoShadowPass, true });
                generateShader(shadowPassFeatures);
            }
        }
        layer.removeChild(model);
    };

    for (const auto &model : models)
        generateShaderForModel(static_cast<QSSGRenderModel &>(*QQuick3DObjectPrivate::get(model)->spatialNode));

    // Let's generate some shaders for the "free" materials as well.
    QSSGRenderModel model; // dummy
    model.meshPath = QSSGRenderPath("#Cube");
    for (const auto &mat : materials) {
        model.materials = { QQuick3DObjectPrivate::get(mat.ptr)->spatialNode };
        generateShaderForModel(model);
    }


    const bool ret = !qsbc.getEntries().isEmpty();
    if (ret)
        qsbcFiles.push_back(resourceFolderRelative + QDir::separator() + QString::fromLatin1(QSSGShaderCache::shaderCollectionFile()));
    qsbc.unmap();

    auto &children = layer.children;
    for (auto it = children.begin(), end = children.end(); it != end;)
        children.remove(*it++);

    qDeleteAll(nodes);

    return ret;
}
