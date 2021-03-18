/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qssgrenderableobjects_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>

QT_BEGIN_NAMESPACE
struct QSSGRenderableImage;
struct QSSGShaderGeneratorGeneratedShader;
struct QSSGSubsetRenderable;

QSSGSubsetRenderableBase::QSSGSubsetRenderableBase(QSSGRenderableObjectFlags inFlags,
                                                       const QVector3D &inWorldCenterPt,
                                                       const QSSGRef<QSSGRendererImpl> &gen,
                                                       const QSSGRenderSubset &inSubset,
                                                       const QSSGModelContext &inModelContext,
                                                       float inOpacity)
    : QSSGRenderableObject(inFlags, inWorldCenterPt, inModelContext.model.globalTransform, inSubset.bounds)
    , generator(gen)
    , modelContext(inModelContext)
    , subset(inSubset)
    , opacity(inOpacity)
{
}

void QSSGSubsetRenderableBase::renderShadowMapPass(const QVector2D &inCameraVec,
                                                     const QSSGRenderLight *inLight,
                                                     const QSSGRenderCamera &inCamera,
                                                     QSSGShadowMapEntry *inShadowMapEntry) const
{
    const auto &context = generator->context();

    /*
        if ( inLight->m_LightType == RenderLightTypes::Area )
                shader = m_Generator.GetParaboloidDepthShader( m_TessellationMode );
        else if ( inLight->m_LightType == RenderLightTypes::Directional )
                shader = m_Generator.GetOrthographicDepthShader( m_TessellationMode );
        else if ( inLight->m_LightType == RenderLightTypes::Point )
                shader = m_Generator.GetCubeShadowDepthShader( m_TessellationMode );	// This
        will change to include a geometry shader pass.
        */

    const QSSGRef<QSSGRenderableDepthPrepassShader> &shader = (inLight->m_lightType == QSSGRenderLight::Type::Directional) ? generator->getOrthographicDepthShader(tessellationMode)
                                                                                                                                 : generator->getCubeShadowDepthShader(tessellationMode);

    if (shader == nullptr || inShadowMapEntry == nullptr)
        return;

    // for phong and npatch tesselleation we need the normals too
    const QSSGRef<QSSGRenderInputAssembler> &pIA = (tessellationMode == TessellationModeValues::NoTessellation
                                                    || tessellationMode == TessellationModeValues::Linear)
            ? subset.inputAssemblerDepth
            : subset.inputAssembler;

    QMatrix4x4 theModelViewProjection = inShadowMapEntry->m_lightVP * globalTransform;
    // QMatrix4x4 theModelView = inLight->m_GlobalTransform.getInverse() * m_GlobalTransform;

    context->setActiveShader(shader->shader);
    shader->mvp.set(theModelViewProjection);
    shader->cameraPosition.set(inCamera.position);
    shader->globalTransform.set(globalTransform);
    shader->cameraProperties.set(inCameraVec);
    /*
        shader->m_CameraDirection.Set( inCamera.GetDirection() );

        shader->m_shadowMV[0].set( inShadowMapEntry->m_lightCubeView[0] * m_globalTransform );
        shader->m_shadowMV[1].set( inShadowMapEntry->m_lightCubeView[1] * m_globalTransform );
        shader->m_shadowMV[2].set( inShadowMapEntry->m_lightCubeView[2] * m_globalTransform );
        shader->m_shadowMV[3].set( inShadowMapEntry->m_lightCubeView[3] * m_globalTransform );
        shader->m_shadowMV[4].set( inShadowMapEntry->m_lightCubeView[4] * m_globalTransform );
        shader->m_shadowMV[5].set( inShadowMapEntry->m_lightCubeView[5] * m_globalTransform );
        shader->m_projection.set( inCamera.m_projection );
        */

    // tesselation
    if (tessellationMode != TessellationModeValues::NoTessellation) {
        // set uniforms we need
        shader->tessellation.edgeTessLevel.set(subset.edgeTessFactor);
        shader->tessellation.insideTessLevel.set(subset.innerTessFactor);
        // the blend value is hardcoded
        shader->tessellation.phongBlend.set(0.75);
        // set distance range value
        shader->tessellation.distanceRange.set(inCameraVec);
        // disable culling
        shader->tessellation.disableCulling.set(1.0);
    }

    context->setInputAssembler(pIA);
    context->draw(subset.primitiveType, subset.count, subset.offset);
}

void QSSGSubsetRenderableBase::renderDepthPass(const QVector2D &inCameraVec, QSSGRenderableImage *inDisplacementImage, float inDisplacementAmount, QSSGCullFaceMode cullFaceMode)
{
    const auto &context = generator->context();
    QSSGRenderableImage *displacementImage = inDisplacementImage;

    const auto &shader = (subset.primitiveType != QSSGRenderDrawMode::Patches) ? generator->getDepthPrepassShader(displacementImage != nullptr)
                                                                                 : generator->getDepthTessPrepassShader(tessellationMode, displacementImage != nullptr);

    if (shader.isNull())
        return;

    // for phong and npatch tesselleation or displacement mapping we need the normals (and uv's)
    // too
    const auto &pIA = ((tessellationMode == TessellationModeValues::NoTessellation
                        || tessellationMode == TessellationModeValues::Linear) && !displacementImage)
            ? subset.inputAssemblerDepth
            : subset.inputAssembler;

    context->setActiveShader(shader->shader);
    context->solveCullingOptions(cullFaceMode);

    shader->mvp.set(modelContext.modelViewProjection);

    if (displacementImage) {
        // setup image transform
        const QMatrix4x4 &textureTransform = displacementImage->m_image.m_textureTransform;
        const float *dataPtr(textureTransform.data());
        QVector3D offsets(dataPtr[12],
                          dataPtr[13],
                          displacementImage->m_image.m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f : 0.0f);
        QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);
        displacementImage->m_image.m_textureData.m_texture->setTextureWrapS(displacementImage->m_image.m_horizontalTilingMode);
        displacementImage->m_image.m_textureData.m_texture->setTextureWrapT(displacementImage->m_image.m_verticalTilingMode);

        shader->displaceAmount.set(inDisplacementAmount);
        shader->displacementProps.offsets.set(offsets);
        shader->displacementProps.rotations.set(rotations);
        shader->displacementProps.sampler.set(displacementImage->m_image.m_textureData.m_texture.data());
    }

    // tesselation
    if (tessellationMode != TessellationModeValues::NoTessellation) {
        // set uniforms we need
        shader->globalTransform.set(globalTransform);

        if (generator->getLayerRenderData() && generator->getLayerRenderData()->camera)
            shader->cameraPosition.set(generator->getLayerRenderData()->camera->getGlobalPos());
        else if (generator->getLayerRenderData()->camera)
            shader->cameraPosition.set(QVector3D(0.0, 0.0, 1.0));

        shader->tessellation.edgeTessLevel.set(subset.edgeTessFactor);
        shader->tessellation.insideTessLevel.set(subset.innerTessFactor);
        // the blend value is hardcoded
        shader->tessellation.phongBlend.set(0.75);
        // set distance range value
        shader->tessellation.distanceRange.set(inCameraVec);
        // enable culling
        shader->tessellation.disableCulling.set(0.0);
    }

    context->setInputAssembler(pIA);
    context->draw(subset.primitiveType, subset.count, subset.offset);
}

// An interface to the shader generator that is available to the renderables

QSSGSubsetRenderable::QSSGSubsetRenderable(QSSGRenderableObjectFlags inFlags,
                                               const QVector3D &inWorldCenterPt,
                                               const QSSGRef<QSSGRendererImpl> &gen,
                                               const QSSGRenderSubset &inSubset,
                                               const QSSGRenderDefaultMaterial &mat,
                                               const QSSGModelContext &inModelContext,
                                               float inOpacity,
                                               QSSGRenderableImage *inFirstImage,
                                               QSSGShaderDefaultMaterialKey inShaderKey,
                                               const QSSGDataView<QMatrix4x4> &inBoneGlobals)
    : QSSGSubsetRenderableBase(inFlags, inWorldCenterPt, gen, inSubset, inModelContext, inOpacity)
    , material(mat)
    , firstImage(inFirstImage)
    , shaderDescription(inShaderKey)
    , bones(inBoneGlobals)
{
    renderableFlags.setDefaultMaterialMeshSubset(true);
    renderableFlags.setCustom(false);
}

void QSSGSubsetRenderable::render(const QVector2D &inCameraVec, const ShaderFeatureSetList &inFeatureSet)
{
    const auto &context = generator->context();

    const QSSGRef<QSSGShaderGeneratorGeneratedShader> &shader = generator->getShader(*this, inFeatureSet);
    if (shader == nullptr)
        return;

    context->setActiveShader(shader->shader);

    generator->contextInterface()->defaultMaterialShaderGenerator()->setMaterialProperties(shader->shader,
                                                                                             material,
                                                                                             inCameraVec,
                                                                                             modelContext.modelViewProjection,
                                                                                             modelContext.normalMatrix,
                                                                                             modelContext.model.globalTransform,
                                                                                             firstImage,
                                                                                             opacity,
                                                                                             generator->getLayerGlobalRenderProperties(),
                                                                                             renderableFlags.receivesShadows());

    // tesselation
    if (subset.primitiveType == QSSGRenderDrawMode::Patches) {
        shader->tessellation.edgeTessLevel.set(subset.edgeTessFactor);
        shader->tessellation.insideTessLevel.set(subset.innerTessFactor);
        // the blend value is hardcoded
        shader->tessellation.phongBlend.set(0.75);
        // this should finally be based on some user input
        shader->tessellation.distanceRange.set(inCameraVec);
        // enable culling
        shader->tessellation.disableCulling.set(0.0);

        if (subset.wireframeMode) {
            // we need the viewport matrix
            QRect theViewport(context->viewport());
            float matrixData[16] = { float(theViewport.width()) / 2.0f,
                                     0.0f,
                                     0.0f,
                                     0.0f,
                                     0.0f,
                                     float(theViewport.width()) / 2.0f,
                                     0.0f,
                                     0.0f,
                                     0.0f,
                                     0.0f,
                                     1.0f,
                                     0.0f,
                                     float(theViewport.width()) / 2.0f + float(theViewport.x()),
                                     float(theViewport.height()) / 2.0f + float(theViewport.y()),
                                     0.0f,
                                     1.0f };
            QMatrix4x4 vpMatrix(matrixData);
            shader->viewportMatrix.set(vpMatrix);
        }
    }

    context->solveCullingOptions(material.cullMode);
    context->setInputAssembler(subset.inputAssembler);
    context->draw(subset.primitiveType, subset.count, subset.offset);
}

void QSSGSubsetRenderable::renderDepthPass(const QVector2D &inCameraVec)
{
    QSSGRenderableImage *displacementImage = nullptr;
    for (QSSGRenderableImage *theImage = firstImage; theImage != nullptr && displacementImage == nullptr;
         theImage = theImage->m_nextImage) {
        if (theImage->m_mapType == QSSGImageMapTypes::Displacement)
            displacementImage = theImage;
    }
    QSSGSubsetRenderableBase::renderDepthPass(inCameraVec, displacementImage, material.displaceAmount, material.cullMode);
}

QSSGCustomMaterialRenderable::QSSGCustomMaterialRenderable(QSSGRenderableObjectFlags inFlags,
                                                               const QVector3D &inWorldCenterPt,
                                                               const QSSGRef<QSSGRendererImpl> &gen,
                                                               const QSSGRenderSubset &inSubset,
                                                               const QSSGRenderCustomMaterial &mat,
                                                               const QSSGModelContext &inModelContext,
                                                               float inOpacity,
                                                               QSSGRenderableImage *inFirstImage,
                                                               QSSGShaderDefaultMaterialKey inShaderKey)
    : QSSGSubsetRenderableBase(inFlags, inWorldCenterPt, gen, inSubset, inModelContext, inOpacity)
    , material(mat)
    , firstImage(inFirstImage)
    , shaderDescription(inShaderKey)
{
    renderableFlags.setCustomMaterialMeshSubset(true);
}

void QSSGCustomMaterialRenderable::render(const QVector2D & /*inCameraVec*/,
                                            const QSSGLayerRenderData &inLayerData,
                                            const QSSGRenderLayer &inLayer,
                                            const QVector<QSSGRenderLight *> &inLights,
                                            const QSSGRenderCamera &inCamera,
                                            const QSSGRef<QSSGRenderTexture2D> &inDepthTexture,
                                            const QSSGRef<QSSGRenderTexture2D> &inSsaoTexture,
                                            const ShaderFeatureSetList &inFeatureSet)
{
    const auto &contextInterface = generator->contextInterface();
    QSSGCustomMaterialRenderContext theRenderContext(inLayer,
                                                       inLayerData,
                                                       inLights,
                                                       inCamera,
                                                       modelContext.model,
                                                       subset,
                                                       modelContext.modelViewProjection,
                                                       globalTransform,
                                                       modelContext.normalMatrix,
                                                       material,
                                                       inDepthTexture,
                                                       inSsaoTexture,
                                                       shaderDescription,
                                                       firstImage,
                                                       opacity);

    contextInterface->customMaterialSystem()->renderSubset(theRenderContext, inFeatureSet);
}

void QSSGCustomMaterialRenderable::renderDepthPass(const QVector2D &inCameraVec,
                                                     const QSSGRenderLayer & /*inLayer*/,
                                                     const QVector<QSSGRenderLight *> &/*inLights*/,
                                                     const QSSGRenderCamera & /*inCamera*/,
                                                     const QSSGRenderTexture2D * /*inDepthTexture*/)
{

    const auto &contextInterface = generator->contextInterface();
    if (!contextInterface->customMaterialSystem()->renderDepthPrepass(modelContext.modelViewProjection, material, subset)) {
        QSSGRenderableImage *displacementImage = nullptr;
        for (QSSGRenderableImage *theImage = firstImage; theImage != nullptr && displacementImage == nullptr;
             theImage = theImage->m_nextImage) {
            if (theImage->m_mapType == QSSGImageMapTypes::Displacement)
                displacementImage = theImage;
        }

        QSSGSubsetRenderableBase::renderDepthPass(inCameraVec, displacementImage, material.m_displaceAmount, material.cullMode);
    }
}

QT_END_NAMESPACE
