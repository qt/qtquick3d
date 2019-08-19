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


#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffectsystem_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgoffscreenrenderkey_p.h>
//#include <QtQuick3DRuntimeRender/private/qssgrenderplugin.h>
//#include <QtQuick3DRuntimeRender/private/qssgrenderplugingraphobject.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcebufferobjects_p.h>
#include <QtQuick3DUtils/private/qssgperftimer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderrenderlist_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#define QSSG_CACHED_POST_EFFECT
const float QSSG_DEGREES_TO_RADIANS = 0.0174532925199f;
const float QSSG_PI = 3.1415926535897f;
const float QSSG_HALFPI = 1.57079632679489661923f;

QT_BEGIN_NAMESPACE

QSSGLayerRenderData::QSSGLayerRenderData(QSSGRenderLayer &inLayer, const QSSGRef<QSSGRendererImpl> &inRenderer)
    : QSSGLayerRenderPreparationData(inLayer, inRenderer)
    , m_layerTexture(inRenderer->demonContext()->resourceManager())
    , m_temporalAATexture(inRenderer->demonContext()->resourceManager())
    , m_layerDepthTexture(inRenderer->demonContext()->resourceManager())
    , m_layerPrepassDepthTexture(inRenderer->demonContext()->resourceManager())
    , m_layerWidgetTexture(inRenderer->demonContext()->resourceManager())
    , m_layerSsaoTexture(inRenderer->demonContext()->resourceManager())
    , m_layerMultisampleTexture(inRenderer->demonContext()->resourceManager())
    , m_layerMultisamplePrepassDepthTexture(inRenderer->demonContext()->resourceManager())
    , m_layerMultisampleWidgetTexture(inRenderer->demonContext()->resourceManager())
    , m_layerCachedTexture(nullptr)
    , m_advancedBlendDrawTexture(nullptr)
    , m_advancedBlendBlendTexture(nullptr)
    , m_advancedModeDrawFB(nullptr)
    , m_advancedModeBlendFB(nullptr)
    , m_progressiveAAPassIndex(0)
    , m_temporalAAPassIndex(0)
    , m_nonDirtyTemporalAAPassIndex(0)
    , m_textScale(1.0f)
    , m_depthBufferFormat(QSSGRenderTextureFormat::Unknown)
{
}

QSSGLayerRenderData::~QSSGLayerRenderData()
{
    const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->demonContext()->resourceManager());
    if (m_layerCachedTexture && m_layerCachedTexture != m_layerTexture.getTexture())
        theResourceManager->release(m_layerCachedTexture);
    if (m_advancedModeDrawFB) {
        m_advancedModeDrawFB = nullptr;
    }
    if (m_advancedModeBlendFB) {
        m_advancedModeBlendFB = nullptr;
    }
    if (m_advancedBlendBlendTexture)
        m_advancedBlendBlendTexture = nullptr;
    if (m_advancedBlendDrawTexture)
        m_advancedBlendDrawTexture = nullptr;
}
void QSSGLayerRenderData::prepareForRender(const QSize &inViewportDimensions, bool forceDirectRender)
{
    QSSGLayerRenderPreparationData::prepareForRender(inViewportDimensions, forceDirectRender);
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->demonContext()->resourceManager());
    // at that time all values shoud be updated
    renderer->updateCbAoShadow(&layer, camera, m_layerDepthTexture);

    // Generate all necessary lighting keys

    if (thePrepResult.flags.wasLayerDataDirty()) {
        m_progressiveAAPassIndex = 0;
    }

    // Get rid of the layer texture if we aren't rendering to texture this frame.
    if (m_layerTexture.getTexture() && !thePrepResult.flags.shouldRenderToTexture()) {
        if (m_layerCachedTexture && m_layerCachedTexture != m_layerTexture.getTexture()) {
            theResourceManager->release(m_layerCachedTexture);
            m_layerCachedTexture = nullptr;
        }

        m_layerTexture.releaseTexture();
        m_layerDepthTexture.releaseTexture();
        m_layerWidgetTexture.releaseTexture();
        m_layerSsaoTexture.releaseTexture();
        m_layerMultisampleTexture.releaseTexture();
        m_layerMultisamplePrepassDepthTexture.releaseTexture();
        m_layerMultisampleWidgetTexture.releaseTexture();
    }

    if (needsWidgetTexture() == false)
        m_layerWidgetTexture.releaseTexture();

    if (m_layerDepthTexture.getTexture() && !thePrepResult.flags.requiresDepthTexture())
        m_layerDepthTexture.releaseTexture();

    if (m_layerSsaoTexture.getTexture() && !thePrepResult.flags.requiresSsaoPass())
        m_layerSsaoTexture.releaseTexture();

    renderer->layerNeedsFrameClear(*this);

    // Clean up the texture cache if layer dimensions changed
    if (inViewportDimensions.width() != m_previousDimensions.width()
            || inViewportDimensions.height() != m_previousDimensions.height()) {
        m_layerTexture.releaseTexture();
        m_layerDepthTexture.releaseTexture();
        m_layerSsaoTexture.releaseTexture();
        m_layerWidgetTexture.releaseTexture();
        m_layerPrepassDepthTexture.releaseTexture();
        m_temporalAATexture.releaseTexture();
        m_layerMultisampleTexture.releaseTexture();
        m_layerMultisamplePrepassDepthTexture.releaseTexture();
        m_layerMultisampleWidgetTexture.releaseTexture();

        m_previousDimensions.setWidth(inViewportDimensions.width());
        m_previousDimensions.setHeight(inViewportDimensions.height());

        theResourceManager->destroyFreeSizedResources();

        // Effect system uses different resource manager, so clean that up too
        renderer->demonContext()->effectSystem()->getResourceManager()->destroyFreeSizedResources();
    }
}

QSSGRenderTextureFormat QSSGLayerRenderData::getDepthBufferFormat()
{
    if (m_depthBufferFormat == QSSGRenderTextureFormat::Unknown) {
        quint32 theExistingDepthBits = renderer->context()->depthBits();
        quint32 theExistingStencilBits = renderer->context()->stencilBits();
        switch (theExistingDepthBits) {
        case 32:
            m_depthBufferFormat = QSSGRenderTextureFormat::Depth32;
            break;
        case 24:
            //  check if we have stencil bits
            if (theExistingStencilBits > 0)
                m_depthBufferFormat = QSSGRenderTextureFormat::Depth24Stencil8; // currently no stencil usage
            // should be Depth24Stencil8 in
            // this case
            else
                m_depthBufferFormat = QSSGRenderTextureFormat::Depth24;
            break;
        case 16:
            m_depthBufferFormat = QSSGRenderTextureFormat::Depth16;
            break;
        default:
            Q_ASSERT(false);
            m_depthBufferFormat = QSSGRenderTextureFormat::Depth16;
            break;
        }
    }
    return m_depthBufferFormat;
}

QSSGRenderFrameBufferAttachment QSSGLayerRenderData::getFramebufferDepthAttachmentFormat(QSSGRenderTextureFormat depthFormat)
{
    QSSGRenderFrameBufferAttachment fmt = QSSGRenderFrameBufferAttachment::Depth;

    switch (depthFormat.format) {
    case QSSGRenderTextureFormat::Depth16:
    case QSSGRenderTextureFormat::Depth24:
    case QSSGRenderTextureFormat::Depth32:
        fmt = QSSGRenderFrameBufferAttachment::Depth;
        break;
    case QSSGRenderTextureFormat::Depth24Stencil8:
        fmt = QSSGRenderFrameBufferAttachment::DepthStencil;
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    return fmt;
}

void QSSGLayerRenderData::renderClearPass()
{
    QSSGStackPerfTimer ___timer(renderer->demonContext()->performanceTimer(), Q_FUNC_INFO);
    if (camera == nullptr)
        return;

    renderer->beginLayerRender(*this);

    const auto &theContext = renderer->context();
    if (layer.background == QSSGRenderLayer::Background::SkyBox) {
        theContext->setDepthTestEnabled(false); // Draw to every pixel
        theContext->setDepthWriteEnabled(false); // Depth will be cleared in a separate step
        QSSGRef<QSSGSkyBoxShader> shader = renderer->getSkyBoxShader();
        theContext->setActiveShader(shader->shader);
        // Setup constants
        shader->projection.set(camera->projection);
        shader->viewMatrix.set(camera->globalTransform);
        shader->skyboxTexture.set(layer.lightProbe->m_textureData.m_texture.data());
        renderer->renderQuad();
    }

    QSSGRenderClearFlags clearFlags = 0;
    if (!layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)) {
        clearFlags |= QSSGRenderClearValues::Depth;
        clearFlags |= QSSGRenderClearValues::Stencil;
        // Enable depth write for the clear below
        theContext->setDepthWriteEnabled(true);
    }

    if (layer.background == QSSGRenderLayer::Background::SkyBox) {
        theContext->clear(clearFlags);
    } else if (layer.background == QSSGRenderLayer::Background::Color) {
        clearFlags |= QSSGRenderClearValues::Color;
        QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theContext,
                                                                  &QSSGRenderContext::clearColor,
                                                                  &QSSGRenderContext::setClearColor,
                                                                  QVector4D(layer.clearColor, 1.0f));
        theContext->clear(clearFlags);
    } else {
        if (layerPrepResult->flags.requiresTransparentClear()) {
            clearFlags |= QSSGRenderClearValues::Color;
            QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theContext,
                                                                      &QSSGRenderContext::clearColor,
                                                                      &QSSGRenderContext::setClearColor,
                                                                      QVector4D(0.0, 0.0, 0.0, 0.0f));
            theContext->clear(clearFlags);
        }
    }
    renderer->endLayerRender();
}

void QSSGLayerRenderData::renderAoPass()
{
    renderer->beginLayerDepthPassRender(*this);

    const auto &theContext = renderer->context();
    QSSGRef<QSSGDefaultAoPassShader> shader = renderer->getDefaultAoPassShader(getShaderFeatureSet());
    if (shader == nullptr)
        return;

    // Set initial state
    theContext->setBlendingEnabled(false);
    theContext->setDepthWriteEnabled(false);
    theContext->setDepthTestEnabled(false);
    theContext->setActiveShader(shader->shader);

    // Setup constants
    shader->cameraDirection.set(cameraDirection);
    shader->viewMatrix.set(camera->globalTransform);

    shader->depthTexture.set(m_layerDepthTexture.getTexture().data());
    shader->depthSamplerSize.set(
                QVector2D(m_layerDepthTexture->textureDetails().width, m_layerDepthTexture->textureDetails().height));

    // Important uniforms for AO calculations
    QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    shader->cameraProperties.set(theCameraProps);
    shader->aoShadowParams.set();

    // Draw a fullscreen quad
    renderer->renderQuad();

    renderer->endLayerDepthPassRender();
}

void QSSGLayerRenderData::renderFakeDepthMapPass(QSSGRenderTexture2D *theDepthTex, QSSGRenderTextureCube *theDepthCube)
{
    renderer->beginLayerDepthPassRender(*this);

    const auto &theContext = renderer->context();
    QSSGRef<QSSGDefaultAoPassShader> shader = theDepthTex ? renderer->getFakeDepthShader(getShaderFeatureSet())
                                                              : renderer->getFakeCubeDepthShader(getShaderFeatureSet());
    if (shader == nullptr)
        return;

    // Set initial state
    theContext->setBlendingEnabled(false);
    theContext->setDepthWriteEnabled(false);
    theContext->setDepthTestEnabled(false);
    theContext->setActiveShader(shader->shader);

    // Setup constants
    shader->cameraDirection.set(cameraDirection);
    shader->viewMatrix.set(camera->globalTransform);

    shader->depthTexture.set(theDepthTex);
    shader->cubeTexture.set(theDepthCube);
    shader->depthSamplerSize.set(QVector2D(theDepthTex->textureDetails().width, theDepthTex->textureDetails().height));

    // Important uniforms for AO calculations
    QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    shader->cameraProperties.set(theCameraProps);
    shader->aoShadowParams.set();

    // Draw a fullscreen quad
    renderer->renderQuad();
}

namespace {

void computeFrustumBounds(const QSSGRenderCamera &inCamera, const QRectF &inViewPort, QVector3D &ctrBound, QVector3D camVerts[8])
{
    QVector3D camEdges[4];

    const float *dataPtr(inCamera.globalTransform.constData());
    QVector3D camX(dataPtr[0], dataPtr[1], dataPtr[2]);
    QVector3D camY(dataPtr[4], dataPtr[5], dataPtr[6]);
    QVector3D camZ(dataPtr[8], dataPtr[9], dataPtr[10]);

    float tanFOV = tanf(inCamera.verticalFov(inViewPort) * 0.5f);
    float asTanFOV = tanFOV * inViewPort.width() / inViewPort.height();
    camEdges[0] = -asTanFOV * camX + tanFOV * camY + camZ;
    camEdges[1] = asTanFOV * camX + tanFOV * camY + camZ;
    camEdges[2] = asTanFOV * camX - tanFOV * camY + camZ;
    camEdges[3] = -asTanFOV * camX - tanFOV * camY + camZ;

    for (int i = 0; i < 4; ++i) {
        camEdges[i].setX(-camEdges[i].x());
        camEdges[i].setY(-camEdges[i].y());
    }

    camVerts[0] = inCamera.position + camEdges[0] * inCamera.clipNear;
    camVerts[1] = inCamera.position + camEdges[0] * inCamera.clipFar;
    camVerts[2] = inCamera.position + camEdges[1] * inCamera.clipNear;
    camVerts[3] = inCamera.position + camEdges[1] * inCamera.clipFar;
    camVerts[4] = inCamera.position + camEdges[2] * inCamera.clipNear;
    camVerts[5] = inCamera.position + camEdges[2] * inCamera.clipFar;
    camVerts[6] = inCamera.position + camEdges[3] * inCamera.clipNear;
    camVerts[7] = inCamera.position + camEdges[3] * inCamera.clipFar;

    ctrBound = camVerts[0];
    for (int i = 1; i < 8; ++i) {
        ctrBound += camVerts[i];
    }
    ctrBound *= 0.125f;
}

void setupCameraForShadowMap(const QVector2D &/*inCameraVec*/,
                             QSSGRenderContext & /*inContext*/,
                             const QRectF &inViewport,
                             const QSSGRenderCamera &inCamera,
                             const QSSGRenderLight *inLight,
                             QSSGRenderCamera &theCamera)
{
    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    theCamera.clipNear = 1.0f;
    theCamera.clipFar = inLight->m_shadowMapFar;
    // Setup camera projection
    QVector3D inLightPos = inLight->getGlobalPos();
    QVector3D inLightDir = inLight->getDirection();

    if (inLight->flags.testFlag(QSSGRenderLight::Flag::LeftHanded))
        inLightPos.setZ(-inLightPos.z());

    inLightPos -= inLightDir * inCamera.clipNear;
    theCamera.fov = inLight->m_shadowMapFov * QSSG_DEGREES_TO_RADIANS;

    if (inLight->m_lightType == QSSGRenderLight::Type::Directional) {
        QVector3D frustBounds[8], boundCtr;
        computeFrustumBounds(inCamera, inViewport, boundCtr, frustBounds);

        QVector3D forward = inLightDir;
        forward.normalize();
        QVector3D right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0));
        right.normalize();
        QVector3D up = QVector3D::crossProduct(right, forward);
        up.normalize();

        // Calculate bounding box of the scene camera frustum
        float minDistanceZ = std::numeric_limits<float>::max();
        float maxDistanceZ = -std::numeric_limits<float>::max();
        float minDistanceY = std::numeric_limits<float>::max();
        float maxDistanceY = -std::numeric_limits<float>::max();
        float minDistanceX = std::numeric_limits<float>::max();
        float maxDistanceX = -std::numeric_limits<float>::max();
        for (int i = 0; i < 8; ++i) {
            float distanceZ = QVector3D::dotProduct(frustBounds[i], forward);
            if (distanceZ < minDistanceZ)
                minDistanceZ = distanceZ;
            if (distanceZ > maxDistanceZ)
                maxDistanceZ = distanceZ;
            float distanceY = QVector3D::dotProduct(frustBounds[i], up);
            if (distanceY < minDistanceY)
                minDistanceY = distanceY;
            if (distanceY > maxDistanceY)
                maxDistanceY = distanceY;
            float distanceX = QVector3D::dotProduct(frustBounds[i], right);
            if (distanceX < minDistanceX)
                minDistanceX = distanceX;
            if (distanceX > maxDistanceX)
                maxDistanceX = distanceX;
        }

        // Apply bounding box parameters to shadow map camera projection matrix
        // so that the whole scene is fit inside the shadow map
        inLightPos = boundCtr;
        theViewport.setHeight(std::abs(maxDistanceY - minDistanceY));
        theViewport.setWidth(std::abs(maxDistanceX - minDistanceX));
        theCamera.clipNear = -std::abs(maxDistanceZ - minDistanceZ);
        theCamera.clipFar = std::abs(maxDistanceZ - minDistanceZ);
    }

    theCamera.flags.setFlag(QSSGRenderCamera::Flag::LeftHanded, false);

    theCamera.flags.setFlag(QSSGRenderCamera::Flag::Orthographic, inLight->m_lightType == QSSGRenderLight::Type::Directional);
    theCamera.parent = nullptr;
    theCamera.pivot = inLight->pivot;

    if (inLight->m_lightType != QSSGRenderLight::Type::Point) {
        theCamera.lookAt(inLightPos, QVector3D(0, 1.0, 0), inLightPos + inLightDir);
    } else {
        theCamera.lookAt(inLightPos, QVector3D(0, 1.0, 0), QVector3D(0, 0, 0));
    }

    theCamera.calculateGlobalVariables(theViewport);
}
}

void setupCubeShadowCameras(const QSSGRenderLight *inLight, QSSGRenderCamera inCameras[6])
{
    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    QVector3D rotOfs[6];

    Q_ASSERT(inLight != nullptr);
    Q_ASSERT(inLight->m_lightType != QSSGRenderLight::Type::Directional);

    QVector3D inLightPos = inLight->getGlobalPos();
    if (inLight->flags.testFlag(QSSGRenderLight::Flag::LeftHanded))
        inLightPos.setZ(-inLightPos.z());

    rotOfs[0] = QVector3D(0.f, -QSSG_HALFPI, QSSG_PI);
    rotOfs[1] = QVector3D(0.f, QSSG_HALFPI, QSSG_PI);
    rotOfs[2] = QVector3D(QSSG_HALFPI, 0.f, 0.f);
    rotOfs[3] = QVector3D(-QSSG_HALFPI, 0.f, 0.f);
    rotOfs[4] = QVector3D(0.f, QSSG_PI, -QSSG_PI);
    rotOfs[5] = QVector3D(0.f, 0.f, QSSG_PI);

    for (int i = 0; i < 6; ++i) {
        inCameras[i].flags.setFlag(QSSGRenderCamera::Flag::LeftHanded, false);

        inCameras[i].flags.setFlag(QSSGRenderCamera::Flag::Orthographic, false);
        inCameras[i].parent = nullptr;
        inCameras[i].pivot = inLight->pivot;
        inCameras[i].clipNear = 1.0f;
        inCameras[i].clipFar = qMax<float>(2.0f, inLight->m_shadowMapFar);
        inCameras[i].fov = inLight->m_shadowMapFov * QSSG_DEGREES_TO_RADIANS;

        inCameras[i].position = inLightPos;
        inCameras[i].rotation = rotOfs[i];
        inCameras[i].calculateGlobalVariables(theViewport);
    }

    /*
        if ( inLight->m_LightType == RenderLightTypes::Point ) return;

        QVector3D viewDirs[6];
        QVector3D viewUp[6];
        QMatrix3x3 theDirMatrix( inLight->m_GlobalTransform.getUpper3x3() );

        viewDirs[0] = theDirMatrix.transform( QVector3D( 1.f, 0.f, 0.f ) );
        viewDirs[2] = theDirMatrix.transform( QVector3D( 0.f, -1.f, 0.f ) );
        viewDirs[4] = theDirMatrix.transform( QVector3D( 0.f, 0.f, 1.f ) );
        viewDirs[0].normalize();  viewDirs[2].normalize();  viewDirs[4].normalize();
        viewDirs[1] = -viewDirs[0];
        viewDirs[3] = -viewDirs[2];
        viewDirs[5] = -viewDirs[4];

        viewUp[0] = viewDirs[2];
        viewUp[1] = viewDirs[2];
        viewUp[2] = viewDirs[5];
        viewUp[3] = viewDirs[4];
        viewUp[4] = viewDirs[2];
        viewUp[5] = viewDirs[2];

        for (int i = 0; i < 6; ++i)
        {
                inCameras[i].LookAt( inLightPos, viewUp[i], inLightPos + viewDirs[i] );
                inCameras[i].CalculateGlobalVariables( theViewport, QVector2D( theViewport.m_Width,
        theViewport.m_Height ) );
        }
        */
}

inline void renderRenderableShadowMapPass(QSSGLayerRenderData &inData,
                                          QSSGRenderableObject &inObject,
                                          const QVector2D &inCameraProps,
                                          const TShaderFeatureSet &,
                                          quint32 lightIndex,
                                          const QSSGRenderCamera &inCamera)
{
    QSSGShadowMapEntry *pEntry = inData.shadowMapManager->getShadowMapEntry(lightIndex);

    // If the object is marked that it doesn't cast shadows, then skip it.
    if (!inObject.renderableFlags.castsShadows())
        return;

    if (inObject.renderableFlags.isDefaultMaterialMeshSubset())
        static_cast<QSSGSubsetRenderableBase &>(inObject).renderShadowMapPass(inCameraProps, inData.globalLights[lightIndex], inCamera, pEntry);
    else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        static_cast<QSSGSubsetRenderableBase &>(inObject).renderShadowMapPass(inCameraProps, inData.globalLights[lightIndex], inCamera, pEntry);
    } else if (inObject.renderableFlags.isPath()) {
        static_cast<QSSGPathRenderable &>(inObject).renderShadowMapPass(inCameraProps, inData.globalLights[lightIndex], inCamera, pEntry);
    }
}

void QSSGLayerRenderData::renderShadowCubeBlurPass(QSSGResourceFrameBuffer *theFB,
                                                     const QSSGRef<QSSGRenderTextureCube> &target0,
                                                     const QSSGRef<QSSGRenderTextureCube> &target1,
                                                     float filterSz,
                                                     float clipFar)
{
    const auto &theContext = renderer->context();

    QSSGRef<QSSGShadowmapPreblurShader> shaderX = renderer->getCubeShadowBlurXShader();
    QSSGRef<QSSGShadowmapPreblurShader> shaderY = renderer->getCubeShadowBlurYShader();

    if (shaderX == nullptr)
        return;
    if (shaderY == nullptr)
        return;
    // if ( theShader == nullptr ) return;

    // Enable drawing to 6 color attachment buffers for cubemap passes
    qint32 buffers[6] = { 0, 1, 2, 3, 4, 5 };
    QSSGDataView<qint32> bufferList(buffers, 6);
    theContext->setDrawBuffers(bufferList);

    // Attach framebuffer targets
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color0, target1, QSSGRenderTextureCubeFace::CubePosX);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color1, target1, QSSGRenderTextureCubeFace::CubeNegX);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color2, target1, QSSGRenderTextureCubeFace::CubePosY);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color3, target1, QSSGRenderTextureCubeFace::CubeNegY);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color4, target1, QSSGRenderTextureCubeFace::CubePosZ);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color5, target1, QSSGRenderTextureCubeFace::CubeNegZ);

    // Set initial state
    theContext->setBlendingEnabled(false);
    theContext->setDepthWriteEnabled(false);
    theContext->setDepthTestEnabled(false);
    // theContext.SetColorWritesEnabled(true);
    theContext->setActiveShader(shaderX->shader);

    shaderX->cameraProperties.set(QVector2D(filterSz, clipFar));
    shaderX->depthCube.set(target0.data());

    // Draw a fullscreen quad
    renderer->renderQuad();

    theContext->setActiveShader(shaderY->shader);

    // Lather, Rinse, and Repeat for the Y-blur pass
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color0, target0, QSSGRenderTextureCubeFace::CubePosX);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color1, target0, QSSGRenderTextureCubeFace::CubeNegX);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color2, target0, QSSGRenderTextureCubeFace::CubePosY);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color3, target0, QSSGRenderTextureCubeFace::CubeNegY);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color4, target0, QSSGRenderTextureCubeFace::CubePosZ);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color5, target0, QSSGRenderTextureCubeFace::CubeNegZ);

    shaderY->cameraProperties.set(QVector2D(filterSz, clipFar));
    shaderY->depthCube.set(target1.data());

    // Draw a fullscreen quad
    renderer->renderQuad();

    theContext->setDepthWriteEnabled(true);
    theContext->setDepthTestEnabled(true);
    // theContext.SetColorWritesEnabled(false);

    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color0,
                         QSSGRenderTextureOrRenderBuffer(),
                         QSSGRenderTextureCubeFace::CubePosX);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color1,
                         QSSGRenderTextureOrRenderBuffer(),
                         QSSGRenderTextureCubeFace::CubeNegX);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color2,
                         QSSGRenderTextureOrRenderBuffer(),
                         QSSGRenderTextureCubeFace::CubePosY);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color3,
                         QSSGRenderTextureOrRenderBuffer(),
                         QSSGRenderTextureCubeFace::CubeNegY);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color4,
                         QSSGRenderTextureOrRenderBuffer(),
                         QSSGRenderTextureCubeFace::CubePosZ);
    (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color5,
                         QSSGRenderTextureOrRenderBuffer(),
                         QSSGRenderTextureCubeFace::CubeNegZ);

    qint32 null = 0;
    theContext->setDrawBuffers(toDataView(null));
}

void QSSGLayerRenderData::renderShadowMapBlurPass(QSSGResourceFrameBuffer *theFB,
                                                    const QSSGRef<QSSGRenderTexture2D> &target0,
                                                    const QSSGRef<QSSGRenderTexture2D> &target1,
                                                    float filterSz,
                                                    float clipFar)
{
    const auto &theContext = renderer->context();

    QSSGRef<QSSGShadowmapPreblurShader> shaderX = renderer->getOrthoShadowBlurXShader();
    QSSGRef<QSSGShadowmapPreblurShader> shaderY = renderer->getOrthoShadowBlurYShader();

    if (shaderX == nullptr)
        return;
    if (shaderY == nullptr)
        return;

    // Attach framebuffer target
    (*theFB)->attach(QSSGRenderFrameBufferAttachment::Color0, target1);
    //(*theFB)->Attach( QSSGRenderFrameBufferAttachments::DepthStencil, *target1 );

    // Set initial state
    theContext->setBlendingEnabled(false);
    theContext->setDepthWriteEnabled(false);
    theContext->setDepthTestEnabled(false);
    theContext->setColorWritesEnabled(true);
    theContext->setActiveShader(shaderX->shader);

    shaderX->cameraProperties.set(QVector2D(filterSz, clipFar));
    shaderX->depthMap.set(target0.data());

    // Draw a fullscreen quad
    renderer->renderQuad();

    (*theFB)->attach(QSSGRenderFrameBufferAttachment::Color0, target0);
    //(*theFB)->Attach( QSSGRenderFrameBufferAttachments::DepthStencil, *target0 );
    theContext->setActiveShader(shaderY->shader);

    shaderY->cameraProperties.set(QVector2D(filterSz, clipFar));
    shaderY->depthMap.set(target1.data());

    // Draw a fullscreen quad
    renderer->renderQuad();

    theContext->setDepthWriteEnabled(true);
    theContext->setDepthTestEnabled(true);
    theContext->setColorWritesEnabled(false);

    //(*theFB)->Attach( QSSGRenderFrameBufferAttachments::DepthStencil,
    // QSSGRenderTextureOrRenderBuffer() );
    (*theFB)->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
}

void QSSGLayerRenderData::renderShadowMapPass(QSSGResourceFrameBuffer *theFB)
{
    QSSGStackPerfTimer ___timer(renderer->demonContext()->performanceTimer(), Q_FUNC_INFO);

    if (!camera)
        return;

    createShadowMapManager();

    // Check if we have anything to render
    if (opaqueObjects.size() == 0 || globalLights.size() == 0)
        return;

    renderer->beginLayerDepthPassRender(*this);

    const auto &theRenderContext = renderer->context();

    // we may change the viewport
    QSSGRenderContextScopedProperty<QRect> __viewport(*theRenderContext, &QSSGRenderContext::viewport, &QSSGRenderContext::setViewport);

    // disable color writes
    // theRenderContext.SetColorWritesEnabled( false );
    theRenderContext->setColorWritesEnabled(true);
    theRenderContext->setDepthWriteEnabled(true);
    theRenderContext->setCullingEnabled(false);
    theRenderContext->setClearColor(QVector4D(1.0, 1.0, 1.0, 1.0));

    // we render the shadow map with a slight offset to prevent shadow acne and cull the front
    // faces
    QSSGRef<QSSGRenderRasterizerState> rsdefaultstate = new QSSGRenderRasterizerState(theRenderContext, 0.0, 0.0, QSSGRenderFace::Back);
    QSSGRef<QSSGRenderRasterizerState> rsstate = new QSSGRenderRasterizerState(theRenderContext, 1.5, 2.0, QSSGRenderFace::Front);
    theRenderContext->setRasterizerState(rsstate);

    QSSGRenderClearFlags clearFlags(QSSGRenderClearValues::Depth | QSSGRenderClearValues::Stencil
                                      | QSSGRenderClearValues::Color);

    for (int i = 0; i < globalLights.size(); i++) {
        // don't render shadows when not casting
        if (globalLights[i]->m_castShadow == false)
            continue;
        QSSGShadowMapEntry *pEntry = shadowMapManager->getShadowMapEntry(i);
        if (pEntry && pEntry->m_depthMap && pEntry->m_depthCopy && pEntry->m_depthRender) {
            QSSGRenderCamera theCamera;

            QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
            setupCameraForShadowMap(theCameraProps, *renderer->context(), __viewport.m_initialValue, *camera, globalLights[i], theCamera);
            // we need this matrix for the final rendering
            theCamera.calculateViewProjectionMatrix(pEntry->m_lightVP);
            pEntry->m_lightView = theCamera.globalTransform.inverted();

            QSSGTextureDetails theDetails(pEntry->m_depthMap->textureDetails());
            theRenderContext->setViewport(QRect(0, 0, (quint32)theDetails.width, (quint32)theDetails.height));

            (*theFB)->attach(QSSGRenderFrameBufferAttachment::Color0, pEntry->m_depthMap);
            (*theFB)->attach(QSSGRenderFrameBufferAttachment::DepthStencil, pEntry->m_depthRender);
            theRenderContext->clear(clearFlags);

            runRenderPass(renderRenderableShadowMapPass, false, true, true, i, theCamera);
            renderShadowMapBlurPass(theFB, pEntry->m_depthMap, pEntry->m_depthCopy, globalLights[i]->m_shadowFilter, globalLights[i]->m_shadowMapFar);
        } else if (pEntry && pEntry->m_depthCube && pEntry->m_cubeCopy && pEntry->m_depthRender) {
            QSSGRenderCamera theCameras[6];

            setupCubeShadowCameras(globalLights[i], theCameras);

            // pEntry->m_LightView = m_Lights[i]->m_LightType == RenderLightTypes::Point ?
            // QMatrix4x4::createIdentity()
            //	: m_Lights[i]->m_GlobalTransform;
            pEntry->m_lightView = QMatrix4x4();

            QSSGTextureDetails theDetails(pEntry->m_depthCube->textureDetails());
            theRenderContext->setViewport(QRect(0, 0, (quint32)theDetails.width, (quint32)theDetails.height));

            // int passes = m_Lights[i]->m_LightType == RenderLightTypes::Point ? 6 : 5;
            int passes = 6;
            for (int k = 0; k < passes; ++k) {
                // theCameras[k].CalculateViewProjectionMatrix( pEntry->m_LightCubeVP[k] );
                pEntry->m_lightCubeView[k] = theCameras[k].globalTransform.inverted();
                theCameras[k].calculateViewProjectionMatrix(pEntry->m_lightVP);

                // Geometry shader multiplication really doesn't work unless you have a
                // 6-layered 3D depth texture...
                // Otherwise, you have no way to depth test while rendering...
                // which more or less completely defeats the purpose of having a cubemap render
                // target.
                QSSGRenderTextureCubeFace curFace = (QSSGRenderTextureCubeFace)(k + 1);
                //(*theFB)->AttachFace( QSSGRenderFrameBufferAttachments::DepthStencil,
                //*pEntry->m_DepthCube, curFace );
                (*theFB)->attach(QSSGRenderFrameBufferAttachment::DepthStencil, pEntry->m_depthRender);
                (*theFB)->attachFace(QSSGRenderFrameBufferAttachment::Color0, pEntry->m_depthCube, curFace);
                (*theFB)->isComplete();
                theRenderContext->clear(clearFlags);

                runRenderPass(renderRenderableShadowMapPass, false, true, true, i, theCameras[k]);
            }

            renderShadowCubeBlurPass(theFB,
                                     pEntry->m_depthCube,
                                     pEntry->m_cubeCopy,
                                     globalLights[i]->m_shadowFilter,
                                     globalLights[i]->m_shadowMapFar);
        }
    }

    (*theFB)->attach(QSSGRenderFrameBufferAttachment::Depth, QSSGRenderTextureOrRenderBuffer());
    (*theFB)->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());

    // enable color writes
    theRenderContext->setColorWritesEnabled(true);
    theRenderContext->setCullingEnabled(true);
    theRenderContext->setClearColor(QVector4D(0.0, 0.0, 0.0, 0.0));
    // reset rasterizer state
    theRenderContext->setRasterizerState(rsdefaultstate);

    renderer->endLayerDepthPassRender();
}

inline void renderRenderableDepthPass(QSSGLayerRenderData &inData,
                                      QSSGRenderableObject &inObject,
                                      const QVector2D &inCameraProps,
                                      const TShaderFeatureSet &,
                                      quint32,
                                      const QSSGRenderCamera &inCamera)
{
    if (inObject.renderableFlags.isDefaultMaterialMeshSubset())
        static_cast<QSSGSubsetRenderable &>(inObject).renderDepthPass(inCameraProps);
    else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        static_cast<QSSGCustomMaterialRenderable &>(inObject).renderDepthPass(inCameraProps, inData.layer, inData.globalLights, inCamera, nullptr);
    } else if (inObject.renderableFlags.isPath()) {
        static_cast<QSSGPathRenderable &>(inObject).renderDepthPass(inCameraProps, inData.layer, inData.globalLights, inCamera, nullptr);
    } else {
        Q_ASSERT(false);
    }
}

void QSSGLayerRenderData::renderDepthPass(bool inEnableTransparentDepthWrite)
{
    QSSGStackPerfTimer ___timer(renderer->demonContext()->performanceTimer(), Q_FUNC_INFO);
    if (camera == nullptr)
        return;

    // Avoid running this method if possible.
    if ((inEnableTransparentDepthWrite == false && (opaqueObjects.size() == 0 || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)))
            || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest))
        return;

    renderer->beginLayerDepthPassRender(*this);

    const auto &theRenderContext = renderer->context();

    // disable color writes
    theRenderContext->setColorWritesEnabled(false);
    theRenderContext->setDepthWriteEnabled(true);

    QSSGRenderClearFlags clearFlags(QSSGRenderClearValues::Stencil | QSSGRenderClearValues::Depth);
    theRenderContext->clear(clearFlags);

    runRenderPass(renderRenderableDepthPass, false, true, inEnableTransparentDepthWrite, 0, *camera);

    // enable color writes
    theRenderContext->setColorWritesEnabled(true);

    renderer->endLayerDepthPassRender();
}

inline void renderRenderable(QSSGLayerRenderData &inData,
                             QSSGRenderableObject &inObject,
                             const QVector2D &inCameraProps,
                             const TShaderFeatureSet &inFeatureSet,
                             quint32,
                             const QSSGRenderCamera &inCamera)
{
    if (inObject.renderableFlags.isDefaultMaterialMeshSubset())
        static_cast<QSSGSubsetRenderable &>(inObject).render(inCameraProps, inFeatureSet);
    else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        // PKC : Need a better place to do this.
        QSSGCustomMaterialRenderable &theObject = static_cast<QSSGCustomMaterialRenderable &>(inObject);
        if (!inData.layer.lightProbe && theObject.material.m_iblProbe)
            inData.setShaderFeature(QSSGShaderDefines::lightProbe(), theObject.material.m_iblProbe->m_textureData.m_texture != nullptr);
        else if (inData.layer.lightProbe)
            inData.setShaderFeature(QSSGShaderDefines::lightProbe(), inData.layer.lightProbe->m_textureData.m_texture != nullptr);

        static_cast<QSSGCustomMaterialRenderable &>(inObject).render(inCameraProps,
                                                                       inData,
                                                                       inData.layer,
                                                                       inData.globalLights,
                                                                       inCamera,
                                                                       inData.m_layerDepthTexture,
                                                                       inData.m_layerSsaoTexture,
                                                                       inFeatureSet);
    } else if (inObject.renderableFlags.isPath()) {
        static_cast<QSSGPathRenderable &>(inObject).render(inCameraProps,
                                                             inData.layer,
                                                             inData.globalLights,
                                                             inCamera,
                                                             inData.m_layerDepthTexture,
                                                             inData.m_layerSsaoTexture,
                                                             inFeatureSet);
    } else {
        Q_ASSERT(false);
    }
}

void QSSGLayerRenderData::runRenderPass(TRenderRenderableFunction inRenderFn,
                                          bool inEnableBlending,
                                          bool inEnableDepthWrite,
                                          bool inEnableTransparentDepthWrite,
                                          quint32 indexLight,
                                          const QSSGRenderCamera &inCamera,
                                          QSSGResourceFrameBuffer *theFB)
{
    const auto &theRenderContext = renderer->context();
    theRenderContext->setDepthFunction(QSSGRenderBoolOp::LessThanOrEqual);
    theRenderContext->setBlendingEnabled(false);
    QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    const auto &theOpaqueObjects = getOpaqueRenderableObjects();
    bool usingDepthBuffer = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest) && theOpaqueObjects.size() > 0;

    if (usingDepthBuffer) {
        theRenderContext->setDepthTestEnabled(true);
        theRenderContext->setDepthWriteEnabled(inEnableDepthWrite);
    } else {
        theRenderContext->setDepthWriteEnabled(false);
        theRenderContext->setDepthTestEnabled(false);
    }

    for (const auto &theObject : theOpaqueObjects) {
        QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
        setShaderFeature(QSSGShaderDefines::cgLighting(), globalLights.empty() == false);
        inRenderFn(*this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
    }

    // transparent objects
    if (inEnableBlending || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
        theRenderContext->setBlendingEnabled(true && inEnableBlending);
        theRenderContext->setDepthWriteEnabled(inEnableTransparentDepthWrite);

        const auto theTransparentObjects = getTransparentRenderableObjects();
        // Assume all objects have transparency if the layer's depth test enabled flag is true.
        if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
            for (const auto &theObject : theTransparentObjects) {
                if (!(theObject->renderableFlags.isCompletelyTransparent())) {
#ifdef ADVANCED_BLEND_SW_FALLBACK
                    // SW fallback for advanced blend modes.
                    // Renders transparent objects to a separate FBO and blends them in shader
                    // with the opaque items and background.
                    QSSGRenderDefaultMaterial::MaterialBlendMode blendMode = QSSGRenderDefaultMaterial::MaterialBlendMode::Normal;
                    if (theObject->renderableFlags.isDefaultMaterialMeshSubset())
                        blendMode = static_cast<QSSGSubsetRenderable &>(*theObject).getBlendingMode();
                    bool useBlendFallback = (blendMode == QSSGRenderDefaultMaterial::MaterialBlendMode::Overlay
                                             || blendMode == QSSGRenderDefaultMaterial::MaterialBlendMode::ColorBurn
                                             || blendMode == QSSGRenderDefaultMaterial::MaterialBlendMode::ColorDodge)
                            && !theRenderContext->supportsAdvancedBlendHW()
                            && !theRenderContext->supportsAdvancedBlendHwKHR() && m_layerPrepassDepthTexture.getTexture();
                    if (useBlendFallback)
                        setupDrawFB(true);
#endif
                    QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
                    setShaderFeature(QSSGShaderDefines::cgLighting(), !globalLights.empty());

                    inRenderFn(*this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
#ifdef ADVANCED_BLEND_SW_FALLBACK
                    // SW fallback for advanced blend modes.
                    // Continue blending after transparent objects have been rendered to a FBO
                    if (useBlendFallback) {
                        blendAdvancedToFB(blendMode, true, theFB);
                        // restore blending status
                        theRenderContext->setBlendingEnabled(inEnableBlending);
                        // restore depth test status
                        theRenderContext->setDepthTestEnabled(usingDepthBuffer);
                        theRenderContext->setDepthWriteEnabled(inEnableTransparentDepthWrite);
                    }
#endif
                }
            }
        }
        // If the layer doesn't have depth enabled then we have to render via an alternate route
        // where the transparent objects vector could have both opaque and transparent objects.
        else {
            for (const auto &theObject : theTransparentObjects) {
                if (!(theObject->renderableFlags.isCompletelyTransparent())) {
#ifdef ADVANCED_BLEND_SW_FALLBACK
                    QSSGRenderDefaultMaterial::MaterialBlendMode blendMode = QSSGRenderDefaultMaterial::MaterialBlendMode::Normal;
                    if (theObject->renderableFlags.isDefaultMaterialMeshSubset())
                        blendMode = static_cast<QSSGSubsetRenderable &>(*theObject).getBlendingMode();
                    bool useBlendFallback = (blendMode == QSSGRenderDefaultMaterial::MaterialBlendMode::Overlay
                                             || blendMode == QSSGRenderDefaultMaterial::MaterialBlendMode::ColorBurn
                                             || blendMode == QSSGRenderDefaultMaterial::MaterialBlendMode::ColorDodge)
                            && !theRenderContext->supportsAdvancedBlendHW()
                            && !theRenderContext->supportsAdvancedBlendHwKHR();

                    if (theObject->renderableFlags.hasTransparency()) {
                        theRenderContext->setBlendingEnabled(true && inEnableBlending);
                        // If we have SW fallback for blend mode, render to a FBO and blend back.
                        // Slow as this must be done per-object (transparent and opaque items
                        // are mixed, not batched)
                        if (useBlendFallback)
                            setupDrawFB(false);
                    }
#endif
                    QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
                    setShaderFeature(QSSGShaderDefines::cgLighting(), !globalLights.empty());
                    inRenderFn(*this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
#ifdef ADVANCED_BLEND_SW_FALLBACK
                    if (useBlendFallback) {
                        blendAdvancedToFB(blendMode, false, theFB);
                        // restore blending status
                        theRenderContext->setBlendingEnabled(inEnableBlending);
                    }
#endif
                }
            }
        }
    }
}

void QSSGLayerRenderData::render(QSSGResourceFrameBuffer *theFB)
{
    QSSGStackPerfTimer ___timer(renderer->demonContext()->performanceTimer(), Q_FUNC_INFO);
    if (camera == nullptr)
        return;

    renderer->beginLayerRender(*this);
    runRenderPass(renderRenderable, true, !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass), false, 0, *camera, theFB);
    renderer->endLayerRender();
}

void QSSGLayerRenderData::createGpuProfiler()
{
    if (renderer->context()->supportsTimerQuery()) {
        m_layerProfilerGpu.reset(new QSSGRenderGPUProfiler(renderer->demonContext(), renderer->context()));
    }
}

void QSSGLayerRenderData::startProfiling(QString &nameID, bool sync)
{
    if (m_layerProfilerGpu) {
        m_layerProfilerGpu->startTimer(nameID, false, sync);
    }
}

void QSSGLayerRenderData::endProfiling(QString &nameID)
{
    if (m_layerProfilerGpu) {
        m_layerProfilerGpu->endTimer(nameID);
    }
}

void QSSGLayerRenderData::startProfiling(const char *nameID, bool sync)
{
    if (m_layerProfilerGpu) {
        QString theStr(QString::fromLocal8Bit(nameID));
        m_layerProfilerGpu->startTimer(theStr, false, sync);
    }
}

void QSSGLayerRenderData::endProfiling(const char *nameID)
{
    if (m_layerProfilerGpu) {
        QString theStr(QString::fromLocal8Bit(nameID));
        m_layerProfilerGpu->endTimer(theStr);
    }
}

void QSSGLayerRenderData::addVertexCount(quint32 count)
{
    if (m_layerProfilerGpu) {
        m_layerProfilerGpu->addVertexCount(count);
    }
}

// Assumes the viewport is setup appropriately to render the widget.
void QSSGLayerRenderData::renderRenderWidgets()
{
    if (camera) {
        const auto &theContext = renderer->context();
        for (int idx = 0, end = iRenderWidgets.size(); idx < end; ++idx) {
            QSSGRenderWidgetInterface &theWidget = *iRenderWidgets.at(idx);
            theWidget.render(*renderer, *theContext);
        }
    }
}

#ifdef ADVANCED_BLEND_SW_FALLBACK
void QSSGLayerRenderData::blendAdvancedEquationSwFallback(const QSSGRef<QSSGRenderTexture2D> &drawTexture,
                                                            const QSSGRef<QSSGRenderTexture2D> &layerTexture,
                                                            AdvancedBlendModes blendMode)
{
    const auto &theContext = renderer->context();
    QSSGRef<QSSGAdvancedModeBlendShader> shader = renderer->getAdvancedBlendModeShader(blendMode);
    if (shader == nullptr)
        return;

    theContext->setActiveShader((shader->shader));

    shader->baseLayer.set(layerTexture.data());
    shader->blendLayer.set(drawTexture.data());
    // Draw a fullscreen quad
    renderer->renderQuad();
}

void QSSGLayerRenderData::setupDrawFB(bool depthEnabled)
{
    const auto &theRenderContext = renderer->context();
    // create drawing FBO and texture, if not existing
    if (!m_advancedModeDrawFB)
        m_advancedModeDrawFB = new QSSGRenderFrameBuffer(theRenderContext);
    if (!m_advancedBlendDrawTexture) {
        m_advancedBlendDrawTexture = new QSSGRenderTexture2D(theRenderContext);
        QRect theViewport = renderer->demonContext()->renderList()->getViewport();
        m_advancedBlendDrawTexture->setTextureData(QSSGByteView(), 0, theViewport.width(), theViewport.height(), QSSGRenderTextureFormat::RGBA8);
        m_advancedModeDrawFB->attach(QSSGRenderFrameBufferAttachment::Color0, m_advancedBlendDrawTexture);
        // Use existing depth prepass information when rendering transparent objects to a FBO
        if (depthEnabled)
            m_advancedModeDrawFB->attach(QSSGRenderFrameBufferAttachment::Depth, m_layerPrepassDepthTexture.getTexture());
    }
    theRenderContext->setRenderTarget(m_advancedModeDrawFB);
    // make sure that depth testing is on in order to render just the
    // depth-passed objects (=transparent objects) and leave background intact
    if (depthEnabled)
        theRenderContext->setDepthTestEnabled(true);
    theRenderContext->setBlendingEnabled(false);
    // clear color commonly is the layer background, make sure that it is all-zero here
    QVector4D originalClrColor = theRenderContext->clearColor();
    theRenderContext->setClearColor(QVector4D(0.0, 0.0, 0.0, 0.0));
    theRenderContext->clear(QSSGRenderClearValues::Color);
    theRenderContext->setClearColor(originalClrColor);
}
void QSSGLayerRenderData::blendAdvancedToFB(QSSGRenderDefaultMaterial::MaterialBlendMode blendMode, bool depthEnabled, QSSGResourceFrameBuffer *theFB)
{
    const auto &theRenderContext = renderer->context();
    QRect theViewport = renderer->demonContext()->renderList()->getViewport();
    AdvancedBlendModes advancedMode;

    switch (blendMode) {
    case QSSGRenderDefaultMaterial::MaterialBlendMode::Overlay:
        advancedMode = AdvancedBlendModes::Overlay;
        break;
    case QSSGRenderDefaultMaterial::MaterialBlendMode::ColorBurn:
        advancedMode = AdvancedBlendModes::ColorBurn;
        break;
    case QSSGRenderDefaultMaterial::MaterialBlendMode::ColorDodge:
        advancedMode = AdvancedBlendModes::ColorDodge;
        break;
    default:
        Q_UNREACHABLE();
    }
    // create blending FBO and texture if not existing
    if (!m_advancedModeBlendFB)
        m_advancedModeBlendFB = new QSSGRenderFrameBuffer(theRenderContext);
    if (!m_advancedBlendBlendTexture) {
        m_advancedBlendBlendTexture = new QSSGRenderTexture2D(theRenderContext);
        m_advancedBlendBlendTexture->setTextureData(QSSGByteView(), 0, theViewport.width(), theViewport.height(), QSSGRenderTextureFormat::RGBA8);
        m_advancedModeBlendFB->attach(QSSGRenderFrameBufferAttachment::Color0, m_advancedBlendBlendTexture);
    }
    theRenderContext->setRenderTarget(m_advancedModeBlendFB);

    // Blend transparent objects with SW fallback shaders.
    // Disable depth testing as transparent objects have already been
    // depth-checked; here we want to run shader for all layer pixels
    if (depthEnabled) {
        theRenderContext->setDepthTestEnabled(false);
        theRenderContext->setDepthWriteEnabled(false);
    }
    blendAdvancedEquationSwFallback(m_advancedBlendDrawTexture, m_layerTexture, advancedMode);
    theRenderContext->setRenderTarget(*theFB);
    // setup read target
    theRenderContext->setReadTarget(m_advancedModeBlendFB);
    theRenderContext->setReadBuffer(QSSGReadFace::Color0);
    theRenderContext->blitFramebuffer(0,
                                      0,
                                      theViewport.width(),
                                      theViewport.height(),
                                      0,
                                      0,
                                      theViewport.width(),
                                      theViewport.height(),
                                      QSSGRenderClearValues::Color,
                                      QSSGRenderTextureMagnifyingOp::Nearest);
}
#endif

// These are meant to be pixel offsets, so you need to divide them by the width/height
// of the layer respectively.
const QVector2D s_VertexOffsets[QSSGLayerRenderPreparationData::MAX_AA_LEVELS] = {
    QVector2D(-0.170840f, -0.553840f), // 1x
    QVector2D(0.162960f, -0.319340f), // 2x
    QVector2D(0.360260f, -0.245840f), // 3x
    QVector2D(-0.561340f, -0.149540f), // 4x
    QVector2D(0.249460f, 0.453460f), // 5x
    QVector2D(-0.336340f, 0.378260f), // 6x
    QVector2D(0.340000f, 0.166260f), // 7x
    QVector2D(0.235760f, 0.527760f), // 8x
};

// Blend factors are in the form of (frame blend factor, accumulator blend factor)
const QVector2D s_BlendFactors[QSSGLayerRenderPreparationData::MAX_AA_LEVELS] = {
    QVector2D(0.500000f, 0.500000f), // 1x
    QVector2D(0.333333f, 0.666667f), // 2x
    QVector2D(0.250000f, 0.750000f), // 3x
    QVector2D(0.200000f, 0.800000f), // 4x
    QVector2D(0.166667f, 0.833333f), // 5x
    QVector2D(0.142857f, 0.857143f), // 6x
    QVector2D(0.125000f, 0.875000f), // 7x
    QVector2D(0.111111f, 0.888889f), // 8x
};

const QVector2D s_TemporalVertexOffsets[QSSGLayerRenderPreparationData::MAX_TEMPORAL_AA_LEVELS] = { QVector2D(.3f, .3f),
                                                                                                      QVector2D(-.3f, -.3f) };

static inline void offsetProjectionMatrix(QMatrix4x4 &inProjectionMatrix, QVector2D inVertexOffsets)
{
    inProjectionMatrix(3, 0) = inProjectionMatrix(3, 0) + inProjectionMatrix(3, 3) * inVertexOffsets.x();
    inProjectionMatrix(3, 1) = inProjectionMatrix(3, 1) + inProjectionMatrix(3, 3) * inVertexOffsets.y();
}

// Render this layer's data to a texture.  Required if we have any effects,
// prog AA, or if forced.
void QSSGLayerRenderData::renderToTexture()
{
    Q_ASSERT(layerPrepResult->flags.shouldRenderToTexture());
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    const auto &theRenderContext = renderer->context();
    QSize theLayerTextureDimensions = thePrepResult.textureDimensions();
    QSize theLayerOriginalTextureDimensions = theLayerTextureDimensions;
    QSSGRenderTextureFormat DepthTextureFormat = QSSGRenderTextureFormat::Depth24Stencil8;
    QSSGRenderTextureFormat ColorTextureFormat = QSSGRenderTextureFormat::RGBA8;
    if (thePrepResult.lastEffect && theRenderContext->renderContextType() != QSSGRenderContextType::GLES2) {
        if (layer.background != QSSGRenderLayer::Background::Transparent)
            ColorTextureFormat = QSSGRenderTextureFormat::R11G11B10;
        else
            ColorTextureFormat = QSSGRenderTextureFormat::RGBA16F;
    }
    QSSGRenderTextureFormat ColorSSAOTextureFormat = QSSGRenderTextureFormat::RGBA8;

    bool needsRender = false;
    qint32 sampleCount = 1;
    // check multsample mode and MSAA texture support
    if (layer.multisampleAAMode != QSSGRenderLayer::AAMode::NoAA && theRenderContext->supportsMultisampleTextures())
        sampleCount = qint32(layer.multisampleAAMode);

    bool isMultisamplePass = false;
    if (theRenderContext->renderContextType() != QSSGRenderContextType::GLES2)
        isMultisamplePass = (sampleCount > 1) || (layer.multisampleAAMode == QSSGRenderLayer::AAMode::SSAA);

    QSSGRenderTextureTargetType thFboAttachTarget = QSSGRenderTextureTargetType::Texture2D;

    // If the user has disabled all layer caching this has the side effect of disabling the
    // progressive AA algorithm.
    if (thePrepResult.flags.wasLayerDataDirty() || thePrepResult.flags.wasDirty()
            || renderer->isLayerCachingEnabled() == false || thePrepResult.flags.shouldRenderToTexture()) {
        m_progressiveAAPassIndex = 0;
        m_nonDirtyTemporalAAPassIndex = 0;
        needsRender = true;
    }

    QSSGResourceTexture2D *renderColorTexture = &m_layerTexture;
    QSSGResourceTexture2D *renderPrepassDepthTexture = &m_layerPrepassDepthTexture;
    QSSGRenderContextScopedProperty<bool> __multisampleEnabled(*theRenderContext,
                                                                 &QSSGRenderContext::isMultisampleEnabled,
                                                                 &QSSGRenderContext::setMultisampleEnabled);
    theRenderContext->setMultisampleEnabled(false);
    if (isMultisamplePass) {
        renderColorTexture = &m_layerMultisampleTexture;
        renderPrepassDepthTexture = &m_layerMultisamplePrepassDepthTexture;
        // for SSAA we don't use MS textures
        if (layer.multisampleAAMode != QSSGRenderLayer::AAMode::SSAA)
            thFboAttachTarget = QSSGRenderTextureTargetType::Texture2D_MS;
    }
    quint32 maxTemporalPassIndex = layer.temporalAAEnabled ? 2 : 0;

    // If all the dimensions match then we do not have to re-render the layer.
    if (m_layerTexture.textureMatches(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), ColorTextureFormat)
            && (!thePrepResult.flags.requiresDepthTexture()
                || m_layerDepthTexture.textureMatches(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), DepthTextureFormat))
            && m_progressiveAAPassIndex >= thePrepResult.maxAAPassIndex
            && m_nonDirtyTemporalAAPassIndex >= maxTemporalPassIndex && needsRender == false) {
        return;
    }

    // adjust render size for SSAA
    if (layer.multisampleAAMode == QSSGRenderLayer::AAMode::SSAA) {
        qint32 ow, oh;
        QSSGRendererUtil::getSSAARenderSize(theLayerOriginalTextureDimensions.width(),
                                              theLayerOriginalTextureDimensions.height(),
                                              ow,
                                              oh);
        theLayerTextureDimensions = QSize(ow, oh);
    }

    // If our pass index == thePreResult.m_MaxAAPassIndex then
    // we shouldn't get into here.

    const QSSGRef<QSSGResourceManager> &theResourceManager = renderer->demonContext()->resourceManager();
    bool hadLayerTexture = true;

    if (renderColorTexture->ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), ColorTextureFormat, sampleCount)) {
        m_progressiveAAPassIndex = 0;
        m_nonDirtyTemporalAAPassIndex = 0;
        hadLayerTexture = false;
    }

    if (thePrepResult.flags.requiresDepthTexture()) {
        // The depth texture doesn't need to be multisample, the prepass depth does.
        if (m_layerDepthTexture.ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), DepthTextureFormat)) {
            // Depth textures are generally not bilinear filtered.
            m_layerDepthTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Nearest);
            m_layerDepthTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Nearest);
            m_progressiveAAPassIndex = 0;
            m_nonDirtyTemporalAAPassIndex = 0;
        }
    }

    if (thePrepResult.flags.requiresSsaoPass()) {
        if (m_layerSsaoTexture.ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), ColorSSAOTextureFormat)) {
            m_layerSsaoTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            m_layerSsaoTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            m_progressiveAAPassIndex = 0;
            m_nonDirtyTemporalAAPassIndex = 0;
        }
    }

    Q_ASSERT(!thePrepResult.flags.requiresDepthTexture() || m_layerDepthTexture.getTexture());
    Q_ASSERT(!thePrepResult.flags.requiresSsaoPass() || m_layerSsaoTexture.getTexture());

    QSSGResourceTexture2D theLastLayerTexture(theResourceManager);
    QSSGRef<QSSGLayerProgAABlendShader> theBlendShader = nullptr;
    quint32 aaFactorIndex = 0;
    bool isProgressiveAABlendPass = m_progressiveAAPassIndex && m_progressiveAAPassIndex < thePrepResult.maxAAPassIndex;
    bool isTemporalAABlendPass = layer.temporalAAEnabled && m_progressiveAAPassIndex == 0;

    if (isProgressiveAABlendPass || isTemporalAABlendPass) {
        theBlendShader = renderer->getLayerProgAABlendShader();
        if (theBlendShader) {
            m_layerTexture.ensureTexture(theLayerOriginalTextureDimensions.width(),
                                         theLayerOriginalTextureDimensions.height(),
                                         ColorTextureFormat);
            QVector2D theVertexOffsets;
            if (isProgressiveAABlendPass) {
                theLastLayerTexture.stealTexture(m_layerTexture);
                aaFactorIndex = (m_progressiveAAPassIndex - 1);
                theVertexOffsets = s_VertexOffsets[aaFactorIndex];
            } else {
                if (m_temporalAATexture.getTexture())
                    theLastLayerTexture.stealTexture(m_temporalAATexture);
                else {
                    if (hadLayerTexture) {
                        theLastLayerTexture.stealTexture(m_layerTexture);
                    }
                }
                theVertexOffsets = s_TemporalVertexOffsets[m_temporalAAPassIndex];
                ++m_temporalAAPassIndex;
                ++m_nonDirtyTemporalAAPassIndex;
                m_temporalAAPassIndex = m_temporalAAPassIndex % MAX_TEMPORAL_AA_LEVELS;
            }
            if (theLastLayerTexture.getTexture()) {
                theVertexOffsets.setX(theVertexOffsets.x() / (theLayerOriginalTextureDimensions.width() / 2.0f));
                theVertexOffsets.setY(theVertexOffsets.y() / (theLayerOriginalTextureDimensions.height() / 2.0f));
                // Run through all models and update MVP.
                // run through all texts and update MVP.
                // run through all path and update MVP.

                // TODO - optimize this exact matrix operation.
                for (qint32 idx = 0, end = modelContexts.size(); idx < end; ++idx) {
                    QMatrix4x4 &originalProjection(modelContexts[idx]->modelViewProjection);
                    offsetProjectionMatrix(originalProjection, theVertexOffsets);
                }
                for (qint32 idx = 0, end = opaqueObjects.size(); idx < end; ++idx) {
                    if (opaqueObjects[idx]->renderableFlags.isPath()) {
                        QSSGPathRenderable &theRenderable = static_cast<QSSGPathRenderable &>(*opaqueObjects[idx]);
                        offsetProjectionMatrix(theRenderable.m_mvp, theVertexOffsets);
                    }
                }
                for (qint32 idx = 0, end = transparentObjects.size(); idx < end; ++idx) {
                    if (transparentObjects[idx]->renderableFlags.isPath()) {
                        QSSGPathRenderable &theRenderable = static_cast<QSSGPathRenderable &>(*transparentObjects[idx]);
                        offsetProjectionMatrix(theRenderable.m_mvp, theVertexOffsets);
                    }
                }
            }
        }
    }
    if (theLastLayerTexture.getTexture() == nullptr) {
        isProgressiveAABlendPass = false;
        isTemporalAABlendPass = false;
    }
    // Sometimes we will have stolen the render texture.
    renderColorTexture->ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), ColorTextureFormat, sampleCount);

    if (!isTemporalAABlendPass)
        m_temporalAATexture.releaseTexture();

    // Allocating a frame buffer can cause it to be bound, so we need to save state before this
    // happens.
    QSSGRenderContextScopedProperty<QSSGRef<QSSGRenderFrameBuffer>> __framebuf(*theRenderContext,
                                                                                     &QSSGRenderContext::renderTarget,
                                                                                     &QSSGRenderContext::setRenderTarget);
    // Match the bit depth of the current render target to avoid popping when we switch from aa
    // to non aa layers
    // We have to all this here in because once we change the FB by allocating an FB we are
    // screwed.
    QSSGRenderTextureFormat theDepthFormat(getDepthBufferFormat());
    QSSGRenderFrameBufferAttachment theDepthAttachmentFormat(getFramebufferDepthAttachmentFormat(theDepthFormat));

    // Definitely disable the scissor rect if it is running right now.
    QSSGRenderContextScopedProperty<bool> __scissorEnabled(*theRenderContext,
                                                             &QSSGRenderContext::isScissorTestEnabled,
                                                             &QSSGRenderContext::setScissorTestEnabled,
                                                             false);
    QSSGResourceFrameBuffer theFB(theResourceManager);
    // Allocates the frame buffer which has the side effect of setting the current render target
    // to that frame buffer.
    // TODO:
    theFB.ensureFrameBuffer();

    bool hasDepthObjects = opaqueObjects.size() > 0;
    bool requiresDepthStencilBuffer = hasDepthObjects || thePrepResult.flags.requiresStencilBuffer();
    QRect theNewViewport(0, 0, theLayerTextureDimensions.width(), theLayerTextureDimensions.height());
    {
        theRenderContext->setRenderTarget(theFB);
        QSSGRenderContextScopedProperty<QRect> __viewport(*theRenderContext,
                                                            &QSSGRenderContext::viewport,
                                                            &QSSGRenderContext::setViewport,
                                                            theNewViewport);
        QVector4D clearColor(0.0, 0.0, 0.0, 0.0);
        if (layer.background == QSSGRenderLayer::Background::Color)
            clearColor = QVector4D(layer.clearColor, 1.0);

        QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theRenderContext,
                                                                  &QSSGRenderContext::clearColor,
                                                                  &QSSGRenderContext::setClearColor,
                                                                  clearColor);
        if (requiresDepthStencilBuffer) {
            if (renderPrepassDepthTexture->ensureTexture(theLayerTextureDimensions.width(),
                                                         theLayerTextureDimensions.height(),
                                                         theDepthFormat,
                                                         sampleCount)) {
                (*renderPrepassDepthTexture)->setMinFilter(QSSGRenderTextureMinifyingOp::Nearest);
                (*renderPrepassDepthTexture)->setMagFilter(QSSGRenderTextureMagnifyingOp::Nearest);
            }
        }

        if (thePrepResult.flags.requiresDepthTexture() && m_progressiveAAPassIndex == 0) {
            // Setup FBO with single depth buffer target.
            // Note this does not use multisample.
            QSSGRenderFrameBufferAttachment theAttachment = getFramebufferDepthAttachmentFormat(DepthTextureFormat);
            theFB->attach(theAttachment, m_layerDepthTexture.getTexture());

            // In this case transparent objects also may write their depth.
            renderDepthPass(true);
            theFB->attach(theAttachment, QSSGRenderTextureOrRenderBuffer());
        }

        if (thePrepResult.flags.requiresSsaoPass() && m_progressiveAAPassIndex == 0 && camera != nullptr) {
            startProfiling("AO pass", false);
            // Setup FBO with single color buffer target
            theFB->attach(QSSGRenderFrameBufferAttachment::Color0, m_layerSsaoTexture.getTexture());
            theRenderContext->clear(QSSGRenderClearValues::Color);
            renderAoPass();
            theFB->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
            endProfiling("AO pass");
        }

        if (thePrepResult.flags.requiresShadowMapPass() && m_progressiveAAPassIndex == 0) {
            // shadow map path
            renderShadowMapPass(&theFB);
        }

        if (sampleCount > 1) {
            theRenderContext->setMultisampleEnabled(true);
        }

        QSSGRenderClearFlags clearFlags = QSSGRenderClearValues::Color;

        // render depth prepass
        if (renderPrepassDepthTexture->getTexture()) {
            theFB->attach(theDepthAttachmentFormat, renderPrepassDepthTexture->getTexture(), thFboAttachTarget);

            if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)) {
                startProfiling("Depth pass", false);
                renderDepthPass(false);
                endProfiling("Depth pass");
            } else {
                clearFlags |= (QSSGRenderClearValues::Depth);
                clearFlags |= (QSSGRenderClearValues::Stencil);
                // enable depth write for the clear below
                theRenderContext->setDepthWriteEnabled(true);
            }
        }

        theFB->attach(QSSGRenderFrameBufferAttachment::Color0, renderColorTexture->getTexture(), thFboAttachTarget);
        if (layer.background != QSSGRenderLayer::Background::Unspecified)
            theRenderContext->clear(clearFlags);

        // We don't clear the depth buffer because the layer render code we are about to call
        // will do this.
        startProfiling("Render pass", false);
        render(&theFB);
        // Debug measure to view the depth map to ensure we're rendering it correctly.
        // if (m_Layer.m_TemporalAAEnabled) {
        //    RenderFakeDepthMapPass(m_ShadowMapManager->GetShadowMapEntry(0)->m_DepthMap,
        //                           m_ShadowMapManager->GetShadowMapEntry(0)->m_DepthCube);
        //}
        endProfiling("Render pass");

        // Now before going further, we downsample and resolve the multisample information.
        // This allows all algorithms running after
        // this point to run unchanged.
        if (isMultisamplePass) {
            if (layer.multisampleAAMode != QSSGRenderLayer::AAMode::SSAA) {
                // Resolve the FBO to the layer texture
                QSSGRendererUtil::resolveMutisampleFBOColorOnly(theResourceManager,
                                                                  m_layerTexture,
                                                                  *theRenderContext,
                                                                  theLayerTextureDimensions.width(),
                                                                  theLayerTextureDimensions.height(),
                                                                  ColorTextureFormat,
                                                                  theFB.getFrameBuffer());

                theRenderContext->setMultisampleEnabled(false);
            } else {
                // Resolve the FBO to the layer texture
                QSSGRendererUtil::resolveSSAAFBOColorOnly(theResourceManager,
                                                            m_layerTexture,
                                                            theLayerOriginalTextureDimensions.width(),
                                                            theLayerOriginalTextureDimensions.height(),
                                                            *theRenderContext,
                                                            theLayerTextureDimensions.width(),
                                                            theLayerTextureDimensions.height(),
                                                            ColorTextureFormat,
                                                            theFB);
            }
        }

        // CN - when I tried to get anti-aliased widgets I lost all transparency on the widget
        // layer which made it overwrite the object you were
        // manipulating.  When I tried to use parallel nsight on it the entire studio
        // application crashed on startup.
        if (needsWidgetTexture()) {
            m_layerWidgetTexture.ensureTexture(theLayerTextureDimensions.width(),
                                               theLayerTextureDimensions.height(),
                                               QSSGRenderTextureFormat::RGBA8);
            theRenderContext->setRenderTarget(theFB);
            theFB->attach(QSSGRenderFrameBufferAttachment::Color0, m_layerWidgetTexture.getTexture());
            theFB->attach(getFramebufferDepthAttachmentFormat(DepthTextureFormat), m_layerDepthTexture.getTexture());
            theRenderContext->setClearColor(QVector4D(0.0, 0.0, 0.0, 0.0));
            theRenderContext->clear(QSSGRenderClearValues::Color);
            // We should already have the viewport and everything setup for this.
            renderRenderWidgets();
        }

        if (theLastLayerTexture.getTexture() != nullptr && (isProgressiveAABlendPass || isTemporalAABlendPass)) {
            theRenderContext->setViewport(
                        QRect(0, 0, theLayerOriginalTextureDimensions.width(), theLayerOriginalTextureDimensions.height()));
            QSSGResourceTexture2D targetTexture(theResourceManager,
                                                  theLayerOriginalTextureDimensions.width(),
                                                  theLayerOriginalTextureDimensions.height(),
                                                  ColorTextureFormat);
            theFB->attach(theDepthAttachmentFormat, QSSGRenderTextureOrRenderBuffer());
            theFB->attach(QSSGRenderFrameBufferAttachment::Color0, targetTexture.getTexture());
            QVector2D theBlendFactors;
            if (isProgressiveAABlendPass)
                theBlendFactors = s_BlendFactors[aaFactorIndex];
            else
                theBlendFactors = QVector2D(.5f, .5f);

            theRenderContext->setDepthTestEnabled(false);
            theRenderContext->setBlendingEnabled(false);
            theRenderContext->setCullingEnabled(false);
            theRenderContext->setActiveShader(theBlendShader->shader);
            theBlendShader->accumSampler.set(theLastLayerTexture.getTexture().data());
            theBlendShader->lastFrame.set(m_layerTexture.getTexture().data());
            theBlendShader->blendFactors.set(theBlendFactors);
            renderer->renderQuad();
            theFB->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
            if (isTemporalAABlendPass)
                m_temporalAATexture.stealTexture(m_layerTexture);
            m_layerTexture.stealTexture(targetTexture);
        }

        m_layerTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
        m_layerTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);

        // Don't remember why needs widget texture is false here.
        // Should have commented why progAA plus widgets is a fail.
        if (m_progressiveAAPassIndex < thePrepResult.maxAAPassIndex && needsWidgetTexture() == false)
            ++m_progressiveAAPassIndex;

        // now we render all post effects
#ifdef QSSG_CACHED_POST_EFFECT
        applyLayerPostEffects();
#endif

        if (m_layerPrepassDepthTexture.getTexture()) {
            // Detach any depth buffers.
            theFB->attach(theDepthAttachmentFormat, QSSGRenderTextureOrRenderBuffer(), thFboAttachTarget);
        }

        theFB->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer(), thFboAttachTarget);
        // Let natural scoping rules destroy the other stuff.
    }
}

void QSSGLayerRenderData::applyLayerPostEffects()
{
    if (layer.firstEffect == nullptr) {
        if (m_layerCachedTexture) {
            const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->demonContext()->resourceManager());
            theResourceManager->release(m_layerCachedTexture);
            m_layerCachedTexture = nullptr;
        }
        return;
    }

    const QSSGRef<QSSGEffectSystem> &theEffectSystem(renderer->demonContext()->effectSystem());
    const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->demonContext()->resourceManager());
    // we use the non MSAA buffer for the effect
    const QSSGRef<QSSGRenderTexture2D> &theLayerColorTexture = m_layerTexture.getTexture();
    const QSSGRef<QSSGRenderTexture2D> &theLayerDepthTexture = m_layerDepthTexture.getTexture();

    QSSGRef<QSSGRenderTexture2D> theCurrentTexture = theLayerColorTexture;
    for (QSSGRenderEffect *theEffect = layer.firstEffect; theEffect; theEffect = theEffect->m_nextEffect) {
        if (theEffect->flags.testFlag(QSSGRenderEffect::Flag::Active) && camera) {
            startProfiling(theEffect->className, false);

            QSSGRef<QSSGRenderTexture2D> theRenderedEffect = theEffectSystem->renderEffect(
                        QSSGEffectRenderArgument(theEffect,
                                                   theCurrentTexture,
                                                   QVector2D(camera->clipNear, camera->clipFar),
                                                   theLayerDepthTexture,
                                                   m_layerPrepassDepthTexture));

            endProfiling(theEffect->className);

            // If the texture came from rendering a chain of effects, then we don't need it
            // after this.
            if (theCurrentTexture != theLayerColorTexture)
                theResourceManager->release(theCurrentTexture);

            theCurrentTexture = theRenderedEffect;

            if (!theRenderedEffect) {
                QString errorMsg = QObject::tr("Failed to compile \"%1\" effect.\nConsider"
                                               " removing it from the presentation.")
                        .arg(QString::fromLatin1(theEffect->className));
                qFatal("%s", errorMsg.toUtf8().constData());
                break;
            }
        }
    }

    if (m_layerCachedTexture && m_layerCachedTexture != m_layerTexture.getTexture()) {
        theResourceManager->release(m_layerCachedTexture);
        m_layerCachedTexture = nullptr;
    }

    if (theCurrentTexture != m_layerTexture.getTexture())
        m_layerCachedTexture = theCurrentTexture;
}

inline bool anyCompletelyNonTransparentObjects(const QSSGLayerRenderPreparationData::TRenderableObjectList &inObjects)
{
    for (int idx = 0, end = inObjects.size(); idx < end; ++idx) {
        if (inObjects[idx]->renderableFlags.isCompletelyTransparent() == false)
            return true;
    }
    return false;
}

void QSSGLayerRenderData::runnableRenderToViewport(const QSSGRef<QSSGRenderFrameBuffer> &theFB)
{
    // If we have an effect, an opaque object, or any transparent objects that aren't completely
    // transparent
    // or an offscreen renderer or a layer widget texture
    // Then we can't possible affect the resulting render target.
    bool needsToRender = layer.firstEffect != nullptr || opaqueObjects.empty() == false
            || anyCompletelyNonTransparentObjects(transparentObjects) || usesOffscreenRenderer()
            || m_layerWidgetTexture.getTexture() || m_boundingRectColor.hasValue()
            || layer.background == QSSGRenderLayer::Background::Color
            || layer.background == QSSGRenderLayer::Background::SkyBox;

    if (needsToRender == false)
        return;

    const auto &theContext = renderer->context();
    theContext->resetStates();

    QSSGRenderContextScopedProperty<QSSGRef<QSSGRenderFrameBuffer>> __fbo(*theContext,
                                                                                &QSSGRenderContext::renderTarget,
                                                                                &QSSGRenderContext::setRenderTarget);
    QRect theCurrentViewport = theContext->viewport();
    QSSGRenderContextScopedProperty<QRect> __viewport(*theContext, &QSSGRenderContext::viewport, &QSSGRenderContext::setViewport);
    QSSGRenderContextScopedProperty<bool> theScissorEnabled(*theContext,
                                                              &QSSGRenderContext::isScissorTestEnabled,
                                                              &QSSGRenderContext::setScissorTestEnabled);
    QSSGRenderContextScopedProperty<QRect> theScissorRect(*theContext,
                                                            &QSSGRenderContext::scissorRect,
                                                            &QSSGRenderContext::setScissorRect);
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    QRectF theScreenRect(thePrepResult.viewport());

    bool blendingEnabled = layer.background == QSSGRenderLayer::Background::Transparent;
    if (!thePrepResult.flags.shouldRenderToTexture()) {
        qint32 sampleCount = 1;
        // check multsample mode and MSAA texture support
        if (layer.multisampleAAMode != QSSGRenderLayer::AAMode::NoAA && theContext->supportsMultisampleTextures())
            sampleCount = qint32(layer.multisampleAAMode);

        // Shadows and SSAO require an FBO, so create one if we are using those
        if (thePrepResult.flags.requiresSsaoPass() || thePrepResult.flags.requiresShadowMapPass()) {
            QSize theLayerTextureDimensions = thePrepResult.textureDimensions();
            QSSGRef<QSSGResourceManager> theResourceManager = renderer->demonContext()->resourceManager();
            QSSGResourceFrameBuffer theFBO(theResourceManager);
            // Allocates the frame buffer which has the side effect of setting the current render target
            // to that frame buffer.
            theFBO.ensureFrameBuffer();

            theContext->setScissorTestEnabled(false);

            if (thePrepResult.flags.requiresSsaoPass()) {
                if (m_layerSsaoTexture.ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), QSSGRenderTextureFormat::RGBA8)) {
                    m_layerSsaoTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
                    m_layerSsaoTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
                    m_progressiveAAPassIndex = 0;
                    m_nonDirtyTemporalAAPassIndex = 0;
                }
            }

            if (thePrepResult.flags.requiresDepthTexture()) {
                // The depth texture doesn't need to be multisample, the prepass depth does.
                if (m_layerDepthTexture.ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), QSSGRenderTextureFormat::Depth24Stencil8)) {
                    // Depth textures are generally not bilinear filtered.
                    m_layerDepthTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Nearest);
                    m_layerDepthTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Nearest);
                    m_progressiveAAPassIndex = 0;
                    m_nonDirtyTemporalAAPassIndex = 0;
                }
            }

            QRect theNewViewport(0, 0, theLayerTextureDimensions.width(), theLayerTextureDimensions.height());
            {
                theContext->setRenderTarget(theFBO);
                QSSGRenderContextScopedProperty<QRect> __viewport(*theContext,
                                                                    &QSSGRenderContext::viewport,
                                                                    &QSSGRenderContext::setViewport,
                                                                    theNewViewport);

                // Depth Prepass with transparent and opaque renderables (for SSAO)
                if (thePrepResult.flags.requiresDepthTexture() && m_progressiveAAPassIndex == 0) {
                    // Setup FBO with single depth buffer target.
                    // Note this does not use multisample.
                    QSSGRenderFrameBufferAttachment theAttachment = getFramebufferDepthAttachmentFormat(QSSGRenderTextureFormat::Depth24Stencil8);
                    theFBO->attach(theAttachment, m_layerDepthTexture.getTexture());

                    // In this case transparent objects also may write their depth.
                    renderDepthPass(true);
                    theFBO->attach(theAttachment, QSSGRenderTextureOrRenderBuffer());
                }

                // SSAO
                if (thePrepResult.flags.requiresSsaoPass() && m_progressiveAAPassIndex == 0 && camera != nullptr) {
                    startProfiling("AO pass", false);
                    // Setup FBO with single color buffer target
                    theFBO->attach(QSSGRenderFrameBufferAttachment::Color0, m_layerSsaoTexture.getTexture());
                    QSSGRenderFrameBufferAttachment theAttachment = getFramebufferDepthAttachmentFormat(QSSGRenderTextureFormat::Depth24Stencil8);
                    theFBO->attach(theAttachment, m_layerDepthTexture.getTexture());
                    theContext->clear(QSSGRenderClearValues::Color);
                    renderAoPass();
                    theFBO->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
                    endProfiling("AO pass");
                }

                // Shadow
                if (thePrepResult.flags.requiresShadowMapPass() && m_progressiveAAPassIndex == 0) {
                    // shadow map path
                    renderShadowMapPass(&theFBO);
                }
            }
        }

        theContext->setRenderTarget(theFB);

        // Multisampling
        if (sampleCount > 1) {
            theContext->setMultisampleEnabled(true);
        }

        // Start Operations on Viewport
        theContext->setViewport(layerPrepResult->viewport().toRect());
        theContext->setScissorTestEnabled(true);
        theContext->setScissorRect(layerPrepResult->scissor().toRect());

        // Viewport Clear
        startProfiling("Clear pass", false);
        renderClearPass();
        endProfiling("Clear pass");

        // Depth Pre-pass
        if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)) {
            startProfiling("Depth pass", false);
            renderDepthPass(false);
            endProfiling("Depth pass");
        }

        // Render pass
        startProfiling("Render pass", false);
        render();
        endProfiling("Render pass");

        // Widget pass
        renderRenderWidgets();

    } else {
        // First, render the layer along with whatever progressive AA is appropriate.
        // The render graph should have taken care of the render to texture step.
#ifdef QSSG_CACHED_POST_EFFECT
        QSSGRef<QSSGRenderTexture2D> theLayerColorTexture = (m_layerCachedTexture) ? m_layerCachedTexture : m_layerTexture;
#else
        // Then render all but the last effect
        IEffectSystem &theEffectSystem(m_Renderer.GetDemonContext().GetEffectSystem());
        IResourceManager &theResourceManager(m_Renderer.GetDemonContext().GetResourceManager());
        // we use the non MSAA buffer for the effect
        QSSGRenderTexture2D *theLayerColorTexture = m_LayerTexture;
        QSSGRenderTexture2D *theLayerDepthTexture = m_LayerDepthTexture;

        QSSGRenderTexture2D *theCurrentTexture = theLayerColorTexture;
        for (SEffect *theEffect = m_Layer.m_FirstEffect; theEffect && theEffect != thePrepResult.m_LastEffect;
             theEffect = theEffect->m_NextEffect) {
            if (theEffect->m_Flags.IsActive() && m_Camera) {
                StartProfiling(theEffect->m_ClassName, false);

                QSSGRenderTexture2D *theRenderedEffect = theEffectSystem.RenderEffect(
                            SEffectRenderArgument(*theEffect,
                                                  *theCurrentTexture,
                                                  QVector2D(m_Camera->m_ClipNear, m_Camera->m_ClipFar),
                                                  theLayerDepthTexture,
                                                  m_LayerPrepassDepthTexture));

                EndProfiling(theEffect->m_ClassName);

                // If the texture came from rendering a chain of effects, then we don't need it
                // after this.
                if (theCurrentTexture != theLayerColorTexture)
                    theResourceManager.Release(*theCurrentTexture);

                theCurrentTexture = theRenderedEffect;
            }
        }
#endif
        // Now the last effect or straight to the scene if we have no last effect
        // There are two cases we need to consider here.  The first is when we shouldn't
        // transform
        // the result and thus we need to setup an MVP that just maps to the viewport width and
        // height.
        // The second is when we are expected to render to the scene using some global
        // transform.
        QMatrix4x4 theFinalMVP;
        QSSGRenderCamera theTempCamera;
        QRect theLayerViewport(thePrepResult.viewport().toRect());
        QRect theLayerClip(thePrepResult.scissor().toRect());

        {
            QMatrix3x3 ignored;
            QMatrix4x4 theViewProjection;
            // We could cache these variables
            theTempCamera.flags.setFlag(QSSGRenderCamera::Flag::Orthographic);
            theTempCamera.markDirty(QSSGRenderCamera::TransformDirtyFlag::TransformIsDirty);
            // Move the camera back far enough that we can see everything
            float theCameraSetback(10);
            // Attempt to ensure the layer can never be clipped.
            theTempCamera.position.setZ(-theCameraSetback);
            theTempCamera.clipFar = 2.0f * theCameraSetback;
            // Render the layer texture to the entire viewport.
            theTempCamera.calculateGlobalVariables(theLayerViewport);
            theTempCamera.calculateViewProjectionMatrix(theViewProjection);
            QSSGRenderNode theTempNode;
            theFinalMVP = theViewProjection;
            QSSGRenderBlendFunctionArgument blendFunc;
            QSSGRenderBlendEquationArgument blendEqu;

            switch (layer.blendType) {
            case QSSGRenderLayer::BlendMode::Screen:
                blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::SrcAlpha,
                                                              QSSGRenderDstBlendFunc::One,
                                                              QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::One);
                blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);
                break;
            case QSSGRenderLayer::BlendMode::Multiply:
                blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::DstColor,
                                                              QSSGRenderDstBlendFunc::Zero,
                                                              QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::One);
                blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);
                break;
            case QSSGRenderLayer::BlendMode::Add:
                blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::One,
                                                              QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::One);
                blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);
                break;
            case QSSGRenderLayer::BlendMode::Subtract:
                blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::One,
                                                              QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::One);
                blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::ReverseSubtract,
                                                             QSSGRenderBlendEquation::ReverseSubtract);
                break;
            case QSSGRenderLayer::BlendMode::Overlay:
                // SW fallback doesn't use blend equation
                // note blend func is not used here anymore
                if (theContext->supportsAdvancedBlendHW() || theContext->supportsAdvancedBlendHwKHR()) {
                    blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Overlay,
                                                                 QSSGRenderBlendEquation::Overlay);
                }
                break;
            case QSSGRenderLayer::BlendMode::ColorBurn:
                // SW fallback doesn't use blend equation
                // note blend func is not used here anymore
                if (theContext->supportsAdvancedBlendHW() || theContext->supportsAdvancedBlendHwKHR()) {
                    blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::ColorBurn,
                                                                 QSSGRenderBlendEquation::ColorBurn);
                }
                break;
            case QSSGRenderLayer::BlendMode::ColorDodge:
                // SW fallback doesn't use blend equation
                // note blend func is not used here anymore
                if (theContext->supportsAdvancedBlendHW() || theContext->supportsAdvancedBlendHwKHR()) {
                    blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::ColorDodge,
                                                                 QSSGRenderBlendEquation::ColorDodge);
                }
                break;
            default:
                blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::OneMinusSrcAlpha,
                                                              QSSGRenderSrcBlendFunc::One,
                                                              QSSGRenderDstBlendFunc::OneMinusSrcAlpha);
                blendEqu = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);
                break;
            }
            theContext->setBlendFunction(blendFunc);
            theContext->setBlendEquation(blendEqu);
            theContext->setBlendingEnabled(blendingEnabled);
            theContext->setDepthTestEnabled(false);
        }

        {
            theContext->setScissorTestEnabled(true);
            theContext->setViewport(theLayerViewport);
            theContext->setScissorRect(theLayerClip);

            // Remember the camera we used so we can get a valid pick ray
            m_sceneCamera = theTempCamera;
            theContext->setDepthTestEnabled(false);
#ifndef QSSG_CACHED_POST_EFFECT
            if (thePrepResult.m_LastEffect && m_Camera) {
                StartProfiling(thePrepResult.m_LastEffect->m_ClassName, false);
                // inUseLayerMPV is true then we are rendering directly to the scene and thus we
                // should enable blending
                // for the final render pass.  Else we should leave it.
                theEffectSystem.RenderEffect(SEffectRenderArgument(*thePrepResult.m_LastEffect,
                                                                   *theCurrentTexture,
                                                                   QVector2D(m_Camera->m_ClipNear, m_Camera->m_ClipFar),
                                                                   theLayerDepthTexture,
                                                                   m_LayerPrepassDepthTexture),
                                             theFinalMVP,
                                             blendingEnabled);
                EndProfiling(thePrepResult.m_LastEffect->m_ClassName);
                // If the texture came from rendering a chain of effects, then we don't need it
                // after this.
                if (theCurrentTexture != theLayerColorTexture)
                    theResourceManager.Release(*theCurrentTexture);
            } else
#endif
            {
                theContext->setCullingEnabled(false);
                theContext->setBlendingEnabled(blendingEnabled);
                theContext->setDepthTestEnabled(false);
#ifdef ADVANCED_BLEND_SW_FALLBACK
                QSSGRef<QSSGRenderTexture2D> screenTexture = renderer->layerBlendTexture();
                QSSGRef<QSSGRenderFrameBuffer> blendFB = renderer->blendFrameBuffer();

                // Layer blending for advanced blending modes if SW fallback is needed
                // rendering to FBO and blending with separate shader
                if (screenTexture) {
                    // Blending is enabled only if layer background has been chosen transparent
                    // Layers with advanced blending modes
                    if (blendingEnabled
                            && (layer.blendType == QSSGRenderLayer::BlendMode::Overlay || layer.blendType == QSSGRenderLayer::BlendMode::ColorBurn
                                || layer.blendType == QSSGRenderLayer::BlendMode::ColorDodge)) {
                        theContext->setScissorTestEnabled(false);
                        theContext->setBlendingEnabled(false);

                        // Get part matching to layer from screen texture and
                        // use that for blending
                        QSSGRef<QSSGRenderTexture2D> blendBlitTexture;
                        blendBlitTexture = new QSSGRenderTexture2D(theContext);
                        blendBlitTexture->setTextureData(QSSGByteView(),
                                                         0,
                                                         theLayerViewport.width(),
                                                         theLayerViewport.height(),
                                                         QSSGRenderTextureFormat::RGBA8);
                        QSSGRef<QSSGRenderFrameBuffer> blitFB;
                        blitFB = new QSSGRenderFrameBuffer(theContext);
                        blitFB->attach(QSSGRenderFrameBufferAttachment::Color0,
                                       QSSGRenderTextureOrRenderBuffer(blendBlitTexture));
                        blendFB->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer(screenTexture));
                        theContext->setRenderTarget(blitFB);
                        theContext->setReadTarget(blendFB);
                        theContext->setReadBuffer(QSSGReadFace::Color0);
                        theContext->blitFramebuffer(theLayerViewport.x(),
                                                    theLayerViewport.y(),
                                                    theLayerViewport.width() + theLayerViewport.x(),
                                                    theLayerViewport.height() + theLayerViewport.y(),
                                                    0,
                                                    0,
                                                    theLayerViewport.width(),
                                                    theLayerViewport.height(),
                                                    QSSGRenderClearValues::Color,
                                                    QSSGRenderTextureMagnifyingOp::Nearest);

                        QSSGRef<QSSGRenderTexture2D> blendResultTexture;
                        blendResultTexture = new QSSGRenderTexture2D(theContext);
                        blendResultTexture->setTextureData(QSSGByteView(),
                                                           0,
                                                           theLayerViewport.width(),
                                                           theLayerViewport.height(),
                                                           QSSGRenderTextureFormat::RGBA8);
                        QSSGRef<QSSGRenderFrameBuffer> resultFB;
                        resultFB = new QSSGRenderFrameBuffer(theContext);
                        resultFB->attach(QSSGRenderFrameBufferAttachment::Color0,
                                         QSSGRenderTextureOrRenderBuffer(blendResultTexture));
                        theContext->setRenderTarget(resultFB);

                        AdvancedBlendModes advancedMode;
                        switch (layer.blendType) {
                        case QSSGRenderLayer::BlendMode::Overlay:
                            advancedMode = AdvancedBlendModes::Overlay;
                            break;
                        case QSSGRenderLayer::BlendMode::ColorBurn:
                            advancedMode = AdvancedBlendModes::ColorBurn;
                            break;
                        case QSSGRenderLayer::BlendMode::ColorDodge:
                            advancedMode = AdvancedBlendModes::ColorDodge;
                            break;
                        default:
                            advancedMode = AdvancedBlendModes::None;
                            break;
                        }

                        theContext->setViewport(QRect(0, 0, theLayerViewport.width(), theLayerViewport.height()));
                        blendAdvancedEquationSwFallback(theLayerColorTexture, blendBlitTexture, advancedMode);
                        // blitFB->release();
                        // save blending result to screen texture for use with other layers
                        theContext->setViewport(theLayerViewport);
                        theContext->setRenderTarget(blendFB);
                        renderer->renderQuad(QVector2D((float)theLayerViewport.width(), (float)theLayerViewport.height()),
                                             theFinalMVP,
                                             *blendResultTexture);
                        // render the blended result
                        theContext->setRenderTarget(theFB);
                        theContext->setScissorTestEnabled(true);
                        renderer->renderQuad(QVector2D((float)theLayerViewport.width(), (float)theLayerViewport.height()),
                                             theFinalMVP,
                                             *blendResultTexture);
                        // resultFB->release();
                    } else {
                        // Layers with normal blending modes
                        // save result for future use
                        theContext->setViewport(theLayerViewport);
                        theContext->setScissorTestEnabled(false);
                        theContext->setBlendingEnabled(true);
                        theContext->setRenderTarget(blendFB);
                        renderer->renderQuad(QVector2D((float)theLayerViewport.width(), (float)theLayerViewport.height()),
                                             theFinalMVP,
                                             *theLayerColorTexture);
                        theContext->setRenderTarget(theFB);
                        theContext->setScissorTestEnabled(true);
                        theContext->setViewport(theCurrentViewport);
                        renderer->renderQuad(QVector2D((float)theLayerViewport.width(), (float)theLayerViewport.height()),
                                             theFinalMVP,
                                             *theLayerColorTexture);
                    }
                } else {
                    // No advanced blending SW fallback needed
                    renderer->renderQuad(QVector2D((float)theLayerViewport.width(), (float)theLayerViewport.height()),
                                         theFinalMVP,
                                         *theLayerColorTexture);
                }
#else
                renderer->renderQuad(QVector2D((float)theLayerViewport.m_Width, (float)theLayerViewport.m_Height),
                                     theFinalMVP,
                                     *theLayerColorTexture);
#endif
            }
            if (m_layerWidgetTexture.getTexture()) {
                theContext->setBlendingEnabled(false);
                renderer->setupWidgetLayer();
                QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
                QRectF thePresRect(thePrepResult.presentationViewport());
                QRectF theLayerRect(thePrepResult.viewport());

                // Ensure we remove any offsetting in the layer rect that was caused simply by
                // the
                // presentation rect offsetting but then use a new rect.
                QRectF theWidgetLayerRect(theLayerRect.x() - thePresRect.x(),
                                          theLayerRect.y() - thePresRect.y(),
                                          theLayerRect.width(),
                                          theLayerRect.height());
                theContext->setScissorTestEnabled(false);
                theContext->setViewport(theWidgetLayerRect.toRect());
                renderer->renderQuad(QVector2D((float)theLayerViewport.width(), (float)theLayerViewport.height()),
                                     theFinalMVP,
                                     *m_layerWidgetTexture);
            }
        }
    } // End offscreen render code.

    if (m_boundingRectColor.hasValue()) {
        QSSGRenderContextScopedProperty<QRect> __viewport(*theContext, &QSSGRenderContext::viewport, &QSSGRenderContext::setViewport);
        QSSGRenderContextScopedProperty<bool> theScissorEnabled(*theContext,
                                                                  &QSSGRenderContext::isScissorTestEnabled,
                                                                  &QSSGRenderContext::setScissorTestEnabled);
        QSSGRenderContextScopedProperty<QRect> theScissorRect(*theContext,
                                                                &QSSGRenderContext::scissorRect,
                                                                &QSSGRenderContext::setScissorRect);
        renderer->setupWidgetLayer();
        // Setup a simple viewport to render to the entire presentation viewport.
        theContext->setViewport(QRect(0,
                                      0,
                                      (quint32)thePrepResult.presentationViewport().width(),
                                      (quint32)thePrepResult.presentationViewport().height()));

        QRectF thePresRect(thePrepResult.presentationViewport());

        // Remove any offsetting from the presentation rect since the widget layer is a
        // stand-alone fbo.
        QRectF theWidgetScreenRect(theScreenRect.x() - thePresRect.x(),
                                   theScreenRect.y() - thePresRect.y(),
                                   theScreenRect.width(),
                                   theScreenRect.height());
        theContext->setScissorTestEnabled(false);
        renderer->drawScreenRect(theWidgetScreenRect, *m_boundingRectColor);
    }
    theContext->setBlendFunction(QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::One,
                                                                   QSSGRenderDstBlendFunc::OneMinusSrcAlpha,
                                                                   QSSGRenderSrcBlendFunc::One,
                                                                   QSSGRenderDstBlendFunc::OneMinusSrcAlpha));
    theContext->setBlendEquation(QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add));
}

void QSSGLayerRenderData::addLayerRenderStep()
{
    QSSGStackPerfTimer __perfTimer(renderer->demonContext()->performanceTimer(), Q_FUNC_INFO);
    Q_ASSERT(camera);
    if (!camera)
        return;

    QSSGRef<QSSGRenderList> theGraph(renderer->demonContext()->renderList());

    QRect theCurrentViewport = theGraph->getViewport();
    if (!layerPrepResult.hasValue())
        prepareForRender(QSize(theCurrentViewport.width(), theCurrentViewport.height()));
}

void QSSGLayerRenderData::prepareForRender()
{
    // When we render to the scene itself (as opposed to an offscreen buffer somewhere)
    // then we use the MVP of the layer somewhat.
    QRect theViewport = renderer->demonContext()->renderList()->getViewport();
    prepareForRender(QSize((quint32)theViewport.width(), (quint32)theViewport.height()));
}

void QSSGLayerRenderData::resetForFrame()
{
    QSSGLayerRenderPreparationData::resetForFrame();
    m_boundingRectColor.setEmpty();
}

void QSSGLayerRenderData::prepareAndRender(const QMatrix4x4 &inViewProjection)
{
    TRenderableObjectList theTransparentObjects(transparentObjects);
    TRenderableObjectList theOpaqueObjects(opaqueObjects);
    theTransparentObjects.clear();
    theOpaqueObjects.clear();
    modelContexts.clear();
    QSSGLayerRenderPreparationResultFlags theFlags;
    prepareRenderablesForRender(inViewProjection, QSSGEmpty(), theFlags);
    renderDepthPass(false);
    render();
}

struct QSSGLayerRenderToTextureRunnable : public QSSGRenderTask
{
    QSSGLayerRenderData &m_data;
    QSSGLayerRenderToTextureRunnable(QSSGLayerRenderData &d) : m_data(d) {}

    void run() override { m_data.renderToTexture(); }
};

static inline QSSGOffscreenRendererDepthValues getOffscreenRendererDepthValue(QSSGRenderTextureFormat inBufferFormat)
{
    switch (inBufferFormat.format) {
    case QSSGRenderTextureFormat::Depth32:
        return QSSGOffscreenRendererDepthValues::Depth32;
    case QSSGRenderTextureFormat::Depth24:
        return QSSGOffscreenRendererDepthValues::Depth24;
    case QSSGRenderTextureFormat::Depth24Stencil8:
        return QSSGOffscreenRendererDepthValues::Depth24;
    default:
        Q_ASSERT(false); // fallthrough intentional
    case QSSGRenderTextureFormat::Depth16:
        return QSSGOffscreenRendererDepthValues::Depth16;
    }
}

QSSGOffscreenRendererEnvironment QSSGLayerRenderData::createOffscreenRenderEnvironment()
{
    QSSGOffscreenRendererDepthValues theOffscreenDepth(getOffscreenRendererDepthValue(getDepthBufferFormat()));
    QRect theViewport = renderer->demonContext()->renderList()->getViewport();
    return QSSGOffscreenRendererEnvironment(theViewport.width(),
                                              theViewport.height(),
                                              QSSGRenderTextureFormat::RGBA8,
                                              theOffscreenDepth,
                                              false,
                                              QSSGRenderLayer::AAMode::NoAA);
}

QSSGRef<QSSGRenderTask> QSSGLayerRenderData::createRenderToTextureRunnable()
{
    return QSSGRef<QSSGRenderTask>(new QSSGLayerRenderToTextureRunnable(*this));
}

QT_END_NAMESPACE
