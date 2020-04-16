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
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcebufferobjects_p.h>
#include <QtQuick3DUtils/private/qssgperftimer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick/QSGTexture>

#include <QtMath>

#define QSSG_CACHED_POST_EFFECT

namespace {
const float QSSG_PI = float(M_PI);
const float QSSG_HALFPI = float(M_PI_2);
}

QT_BEGIN_NAMESPACE

QSSGLayerRenderData::QSSGLayerRenderData(QSSGRenderLayer &inLayer, const QSSGRef<QSSGRendererImpl> &inRenderer)
    : QSSGLayerRenderPreparationData(inLayer, inRenderer)
    , m_layerTexture(inRenderer->contextInterface()->resourceManager())
    , m_temporalAATexture(inRenderer->contextInterface()->resourceManager())
    , m_prevTemporalAATexture(inRenderer->contextInterface()->resourceManager())
    , m_layerDepthTexture(inRenderer->contextInterface()->resourceManager())
    , m_layerPrepassDepthTexture(inRenderer->contextInterface()->resourceManager())
    , m_layerSsaoTexture(inRenderer->contextInterface()->resourceManager())
    , m_layerMultisampleTexture(inRenderer->contextInterface()->resourceManager())
    , m_layerMultisamplePrepassDepthTexture(inRenderer->contextInterface()->resourceManager())
    , m_layerMultisampleWidgetTexture(inRenderer->contextInterface()->resourceManager())
    , m_progressiveAAPassIndex(0)
    , m_temporalAAPassIndex(0)
    , m_nonDirtyTemporalAAPassIndex(0)
    , m_textScale(1.0f)
    , m_depthBufferFormat(QSSGRenderTextureFormat::Unknown)
{
}

QSSGLayerRenderData::~QSSGLayerRenderData()
{
}

void QSSGLayerRenderData::prepareForRender(const QSize &inViewportDimensions)
{
    QSSGLayerRenderPreparationData::prepareForRender(inViewportDimensions);
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->contextInterface()->resourceManager());
    // at that time all values shoud be updated
    renderer->updateCbAoShadow(&layer, camera, m_layerDepthTexture);

    // Generate all necessary lighting keys

    if (thePrepResult.flags.wasLayerDataDirty()) {
        m_progressiveAAPassIndex = 0;
    }

    // Get rid of the layer texture if we aren't rendering to texture this frame.
    if (m_layerTexture.getTexture()) {
        m_layerTexture.releaseTexture();
        m_layerDepthTexture.releaseTexture();
        m_layerSsaoTexture.releaseTexture();
        m_layerMultisampleTexture.releaseTexture();
        m_layerMultisamplePrepassDepthTexture.releaseTexture();
        m_layerMultisampleWidgetTexture.releaseTexture();
    }

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
        m_layerPrepassDepthTexture.releaseTexture();
        m_temporalAATexture.releaseTexture();
        m_layerMultisampleTexture.releaseTexture();
        m_layerMultisamplePrepassDepthTexture.releaseTexture();
        m_layerMultisampleWidgetTexture.releaseTexture();

        m_previousDimensions.setWidth(inViewportDimensions.width());
        m_previousDimensions.setHeight(inViewportDimensions.height());

        theResourceManager->destroyFreeSizedResources();

        // Effect system uses different resource manager, so clean that up too
        renderer->contextInterface()->effectSystem()->getResourceManager()->destroyFreeSizedResources();
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
    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera == nullptr)
        return;

    renderer->beginLayerRender(*this);

    const auto &theContext = renderer->context();
    auto background = layer.background;
    if (background == QSSGRenderLayer::Background::SkyBox) {
        if (layer.lightProbe && !layer.lightProbe->m_textureData.m_texture.isNull()) {
            theContext->setDepthTestEnabled(false); // Draw to every pixel
            theContext->setDepthWriteEnabled(false); // Depth will be cleared in a separate step
            QSSGRef<QSSGSkyBoxShader> shader = renderer->getSkyBoxShader();
            theContext->setActiveShader(shader->shader);
            // Setup constants
            shader->projection.set(camera->projection);
            shader->viewMatrix.set(camera->globalTransform);
            shader->skyboxTexture.set(layer.lightProbe->m_textureData.m_texture.data());
            renderer->renderQuad();
        } else {
            // Revert to color
            background = QSSGRenderLayer::Background::Color;
        }
    }

    QSSGRenderClearFlags clearFlags;
    if (!layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)) {
        clearFlags |= QSSGRenderClearValues::Depth;
        clearFlags |= QSSGRenderClearValues::Stencil;
        // Enable depth write for the clear below
        theContext->setDepthWriteEnabled(true);
    }

    if (background == QSSGRenderLayer::Background::Color) {
        clearFlags |= QSSGRenderClearValues::Color;
        QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theContext,
                                                                  &QSSGRenderContext::clearColor,
                                                                  &QSSGRenderContext::setClearColor,
                                                                  QVector4D(layer.clearColor, 1.0f));
        theContext->clear(clearFlags);
    } else if (layerPrepResult->flags.requiresTransparentClear() &&
               background != QSSGRenderLayer::Background::SkyBox) {
        clearFlags |= QSSGRenderClearValues::Color;
        QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theContext,
                                                                &QSSGRenderContext::clearColor,
                                                                &QSSGRenderContext::setClearColor,
                                                                QVector4D(0.0, 0.0, 0.0, 0.0f));
        theContext->clear(clearFlags);
    } else if (clearFlags) {
        theContext->clear(clearFlags);
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
    shader->depthTextureSize.set(
                QVector2D(m_layerDepthTexture->textureDetails().width, m_layerDepthTexture->textureDetails().height));

    // Important uniforms for AO calculations
    QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    shader->cameraProperties.set(theCameraProps);
    shader->aoShadowParams.set();

    // Draw a fullscreen quad
    renderer->renderQuad();

    renderer->endLayerDepthPassRender();
}

#ifdef QT_QUICK3D_DEBUG_SHADOWS
void QSSGLayerRenderData::renderDebugDepthMap(QSSGRenderTexture2D *theDepthTex, QSSGRenderTextureCube *theDepthCube)
{
    renderer->beginLayerDepthPassRender(*this);

    const auto &theContext = renderer->context();
    QSSGRef<QSSGDefaultAoPassShader> shader = theDepthTex ? renderer->getDebugDepthShader(getShaderFeatureSet())
                                                          : renderer->getDebugCubeDepthShader(getShaderFeatureSet());
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
    shader->depthTextureSize.set(QVector2D(theDepthTex->textureDetails().width, theDepthTex->textureDetails().height));

    // Important uniforms for AO calculations
    QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    shader->cameraProperties.set(theCameraProps);
    shader->aoShadowParams.set();

    // Draw a fullscreen quad
    renderer->renderQuad();

    renderer->endLayerDepthPassRender();
}
#endif

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

QSSGBounds3 calculateShadowCameraBoundingBox(const QVector3D *points, const QVector3D &forward,
                                             const QVector3D &up, const QVector3D &right)
{
    float minDistanceZ = std::numeric_limits<float>::max();
    float maxDistanceZ = -std::numeric_limits<float>::max();
    float minDistanceY = std::numeric_limits<float>::max();
    float maxDistanceY = -std::numeric_limits<float>::max();
    float minDistanceX = std::numeric_limits<float>::max();
    float maxDistanceX = -std::numeric_limits<float>::max();
    for (int i = 0; i < 8; ++i) {
        float distanceZ = QVector3D::dotProduct(points[i], forward);
        if (distanceZ < minDistanceZ)
            minDistanceZ = distanceZ;
        if (distanceZ > maxDistanceZ)
            maxDistanceZ = distanceZ;
        float distanceY = QVector3D::dotProduct(points[i], up);
        if (distanceY < minDistanceY)
            minDistanceY = distanceY;
        if (distanceY > maxDistanceY)
            maxDistanceY = distanceY;
        float distanceX = QVector3D::dotProduct(points[i], right);
        if (distanceX < minDistanceX)
            minDistanceX = distanceX;
        if (distanceX > maxDistanceX)
            maxDistanceX = distanceX;
    }
    return QSSGBounds3(QVector3D(minDistanceX, minDistanceY, minDistanceZ),
                       QVector3D(maxDistanceX, maxDistanceY, maxDistanceZ));
}

void setupCameraForShadowMap(const QVector2D &/*inCameraVec*/,
                             QSSGRenderContext & /*inContext*/,
                             const QRectF &inViewport,
                             const QSSGRenderCamera &inCamera,
                             const QSSGRenderLight *inLight,
                             QSSGRenderCamera &theCamera,
                             QVector3D *scenePoints = nullptr)
{
    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    theCamera.clipNear = 1.0f;
    theCamera.clipFar = inLight->m_shadowMapFar;
    // Setup camera projection
    QVector3D inLightPos = inLight->getGlobalPos();
    QVector3D inLightDir = inLight->getDirection();

    inLightPos -= inLightDir * inCamera.clipNear;
    theCamera.fov = qDegreesToRadians(90.f);

    if (inLight->m_lightType == QSSGRenderLight::Type::Directional) {
        QVector3D frustumPoints[8], boundCtr, sceneCtr;
        computeFrustumBounds(inCamera, inViewport, boundCtr, frustumPoints);

        if (scenePoints) {
            sceneCtr = QVector3D(0, 0, 0);
            for (int i = 0; i < 8; ++i)
                sceneCtr += scenePoints[i];
            sceneCtr *= 0.125f;
        }

        QVector3D forward = inLightDir;
        forward.normalize();
        QVector3D right;
        if (!qFuzzyCompare(qAbs(forward.y()), 1.0f))
            right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0));
        else
            right = QVector3D::crossProduct(forward, QVector3D(1, 0, 0));
        right.normalize();
        QVector3D up = QVector3D::crossProduct(right, forward);
        up.normalize();

        // Calculate bounding box of the scene camera frustum
        QSSGBounds3 bounds = calculateShadowCameraBoundingBox(frustumPoints, forward, up, right);
        inLightPos = boundCtr;
        if (scenePoints) {
            QSSGBounds3 sceneBounds = calculateShadowCameraBoundingBox(scenePoints, forward, up,
                                                                       right);
            if (sceneBounds.extents().x() * sceneBounds.extents().y() * sceneBounds.extents().z()
                    < bounds.extents().x() * bounds.extents().y() * bounds.extents().z()) {
                bounds = sceneBounds;
                inLightPos = sceneCtr;
            }
        }

        // Apply bounding box parameters to shadow map camera projection matrix
        // so that the whole scene is fit inside the shadow map
        theViewport.setHeight(bounds.extents().y() * 2);
        theViewport.setWidth(bounds.extents().x() * 2);
        theCamera.clipNear = -bounds.extents().z() * 2;
        theCamera.clipFar = bounds.extents().z() * 2;
    }

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
    QQuaternion rotOfs[6];

    Q_ASSERT(inLight != nullptr);
    Q_ASSERT(inLight->m_lightType != QSSGRenderLight::Type::Directional);

    const QVector3D inLightPos = inLight->getGlobalPos();

    rotOfs[0] = QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(-QSSG_HALFPI), qRadiansToDegrees(QSSG_PI));
    rotOfs[1] = QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_HALFPI), qRadiansToDegrees(QSSG_PI));
    rotOfs[2] = QQuaternion::fromEulerAngles(qRadiansToDegrees(QSSG_HALFPI), 0.f, 0.f);
    rotOfs[3] = QQuaternion::fromEulerAngles(qRadiansToDegrees(-QSSG_HALFPI), 0.f, 0.f);
    rotOfs[4] = QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_PI), qRadiansToDegrees(-QSSG_PI));
    rotOfs[5] = QQuaternion::fromEulerAngles(0.f, 0.f, qRadiansToDegrees(QSSG_PI));

    for (int i = 0; i < 6; ++i) {
        inCameras[i].flags.setFlag(QSSGRenderCamera::Flag::Orthographic, false);
        inCameras[i].parent = nullptr;
        inCameras[i].pivot = inLight->pivot;
        inCameras[i].clipNear = 1.0f;
        inCameras[i].clipFar = qMax<float>(2.0f, inLight->m_shadowMapFar);
        inCameras[i].fov = qDegreesToRadians(90.f);

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
                                          const ShaderFeatureSetList &,
                                          quint32 lightIndex,
                                          const QSSGRenderCamera &inCamera)
{
    QSSGShadowMapEntry *pEntry = inData.shadowMapManager->getShadowMapEntry(lightIndex);

    // If the object is marked that it doesn't cast shadows, then skip it.
    if (!inObject.renderableFlags.castsShadows())
        return;

    if (inObject.renderableFlags.isDefaultMaterialMeshSubset())
        static_cast<QSSGSubsetRenderableBase &>(inObject).renderShadowMapPass(inCameraProps, inData.globalLights[lightIndex], inCamera, pEntry);
    else if (inObject.renderableFlags.isCustomMaterialMeshSubset())
        static_cast<QSSGSubsetRenderableBase &>(inObject).renderShadowMapPass(inCameraProps, inData.globalLights[lightIndex], inCamera, pEntry);
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
    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);

    if (!camera)
        return;

    if (!shadowMapManager)
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
    QSSGRef<QSSGRenderRasterizerState> rsdefaultstate = new QSSGRenderRasterizerState(theRenderContext, 0.0, 0.0);
    QSSGRef<QSSGRenderRasterizerState> rsstate = new QSSGRenderRasterizerState(theRenderContext, 1.5, 2.0);
    theRenderContext->setRasterizerState(rsstate);

    QSSGRenderClearFlags clearFlags(QSSGRenderClearValues::Depth | QSSGRenderClearValues::Stencil
                                      | QSSGRenderClearValues::Color);

    auto bounds = camera->parent->getBounds(renderer->contextInterface()->bufferManager());

    QVector3D scenePoints[8];
    scenePoints[0] = bounds.minimum;
    scenePoints[1] = QVector3D(bounds.maximum.x(), bounds.minimum.y(), bounds.minimum.z());
    scenePoints[2] = QVector3D(bounds.minimum.x(), bounds.maximum.y(), bounds.minimum.z());
    scenePoints[3] = QVector3D(bounds.maximum.x(), bounds.maximum.y(), bounds.minimum.z());
    scenePoints[4] = QVector3D(bounds.minimum.x(), bounds.minimum.y(), bounds.maximum.z());
    scenePoints[5] = QVector3D(bounds.maximum.x(), bounds.minimum.y(), bounds.maximum.z());
    scenePoints[6] = QVector3D(bounds.minimum.x(), bounds.maximum.y(), bounds.maximum.z());
    scenePoints[7] = bounds.maximum;

    for (int i = 0; i < globalLights.size(); i++) {
        // don't render shadows when not casting
        if (!globalLights[i]->m_castShadow)
            continue;

        QSSGShadowMapEntry *pEntry = shadowMapManager->getShadowMapEntry(i);
        if (pEntry && pEntry->m_depthMap && pEntry->m_depthCopy && pEntry->m_depthRender) {
            QSSGRenderCamera theCamera;

            QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
            setupCameraForShadowMap(theCameraProps, *renderer->context(), __viewport.m_initialValue,
                                    *camera, globalLights[i], theCamera, scenePoints);
            // we need this matrix for the final rendering
            theCamera.calculateViewProjectionMatrix(pEntry->m_lightVP);
            pEntry->m_lightView = theCamera.globalTransform.inverted();

            QSSGTextureDetails theDetails(pEntry->m_depthMap->textureDetails());
            theRenderContext->setViewport(QRect(0, 0, (quint32)theDetails.width, (quint32)theDetails.height));

            (*theFB)->attach(QSSGRenderFrameBufferAttachment::Color0, pEntry->m_depthMap);
            (*theFB)->attach(QSSGRenderFrameBufferAttachment::DepthStencil, pEntry->m_depthRender);
            theRenderContext->clear(clearFlags);

            runRenderPass(renderRenderableShadowMapPass, false, true, true, true, i, theCamera);
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

                runRenderPass(renderRenderableShadowMapPass, false, true, true, true, i, theCameras[k]);
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
                                      const ShaderFeatureSetList &,
                                      quint32,
                                      const QSSGRenderCamera &inCamera)
{
    if (inObject.renderableFlags.isDefaultMaterialMeshSubset())
        static_cast<QSSGSubsetRenderable &>(inObject).renderDepthPass(inCameraProps);
    else if (inObject.renderableFlags.isCustomMaterialMeshSubset())
        static_cast<QSSGCustomMaterialRenderable &>(inObject).renderDepthPass(inCameraProps, inData.layer, inData.globalLights, inCamera, nullptr);
    else
        Q_ASSERT(false);

}

void QSSGLayerRenderData::renderDepthPass(bool inEnableTransparentDepthWrite)
{
    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera == nullptr)
        return;

    // Avoid running this method if possible.
    if ((!inEnableTransparentDepthWrite
         && (opaqueObjects.size() == 0 || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)))
        || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest))
        return;

    renderer->beginLayerDepthPassRender(*this);

    const auto &theRenderContext = renderer->context();

    // disable color writes
    theRenderContext->setColorWritesEnabled(false);
    theRenderContext->setDepthWriteEnabled(true);

    QSSGRenderClearFlags clearFlags(QSSGRenderClearValues::Stencil | QSSGRenderClearValues::Depth);
    theRenderContext->clear(clearFlags);

    runRenderPass(renderRenderableDepthPass, false, true, false, inEnableTransparentDepthWrite, 0, *camera);

    // enable color writes
    theRenderContext->setColorWritesEnabled(true);

    renderer->endLayerDepthPassRender();
}

inline void renderRenderable(QSSGLayerRenderData &inData,
                             QSSGRenderableObject &inObject,
                             const QVector2D &inCameraProps,
                             const ShaderFeatureSetList &inFeatureSet,
                             quint32,
                             const QSSGRenderCamera &inCamera)
{
    if (inObject.renderableFlags.isDefaultMaterialMeshSubset()) {
        static_cast<QSSGSubsetRenderable &>(inObject).render(inCameraProps, inFeatureSet);
    } else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        // PKC : Need a better place to do this.
        QSSGCustomMaterialRenderable &theObject = static_cast<QSSGCustomMaterialRenderable &>(inObject);
        if (!inData.layer.lightProbe && theObject.material.m_iblProbe)
            inData.setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::LightProbe), theObject.material.m_iblProbe->m_textureData.m_texture != nullptr);
        else if (inData.layer.lightProbe)
            inData.setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::LightProbe), inData.layer.lightProbe->m_textureData.m_texture != nullptr);

        static_cast<QSSGCustomMaterialRenderable &>(inObject).render(inCameraProps,
                                                                       inData,
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
                                          bool inSortOpaqueRenderables,
                                          quint32 indexLight,
                                          const QSSGRenderCamera &inCamera,
                                          QSSGResourceFrameBuffer *theFB)
{
    Q_UNUSED(theFB)
    const auto &theRenderContext = renderer->context();
    theRenderContext->setDepthFunction(QSSGRenderBoolOp::LessThanOrEqual);
    theRenderContext->setBlendingEnabled(false);
    QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    const auto &theOpaqueObjects = getOpaqueRenderableObjects(inSortOpaqueRenderables);
    bool usingDepthBuffer = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest) && theOpaqueObjects.size() > 0;

    if (usingDepthBuffer) {
        theRenderContext->setDepthTestEnabled(true);
        theRenderContext->setDepthWriteEnabled(inEnableDepthWrite);
    } else {
        theRenderContext->setDepthWriteEnabled(false);
        theRenderContext->setDepthTestEnabled(false);
    }

    for (const auto &handle : theOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
        setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());
        inRenderFn(*this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
    }

    // Render Quick items
    for (auto theNodeEntry : getRenderableItem2Ds()) {
        QSSGRenderItem2D *item2D = static_cast<QSSGRenderItem2D *>(theNodeEntry.node);
        // Fast-path to avoid rendering totally transparent items
        if (item2D->combinedOpacity < QSSG_RENDER_MINIMUM_RENDER_OPACITY)
            continue;
        // Don't try rendering until texture exists
        if (!item2D->qsgTexture)
            continue;
        QVector2D dimensions = QVector2D(item2D->qsgTexture->textureSize().width(),
                                         item2D->qsgTexture->textureSize().height());
        QSSGRenderTexture2D tex(renderer->context(), item2D->qsgTexture);

        renderer->renderFlippedQuad(dimensions, item2D->MVP, tex, item2D->combinedOpacity);
    }

    // transparent objects
    if (inEnableBlending || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
        theRenderContext->setBlendingEnabled(inEnableBlending);
        theRenderContext->setDepthWriteEnabled(inEnableTransparentDepthWrite);

        const auto &theTransparentObjects = getTransparentRenderableObjects();
        // Assume all objects have transparency if the layer's depth test enabled flag is true.
        if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
            for (const auto &handle : theTransparentObjects) {
                QSSGRenderableObject *theObject = handle.obj;
                if (!(theObject->renderableFlags.isCompletelyTransparent())) {
                    QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
                    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());

                    inRenderFn(*this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
                }
            }
        }
        // If the layer doesn't have depth enabled then we have to render via an alternate route
        // where the transparent objects vector could have both opaque and transparent objects.
        else {
            for (const auto &handle : theTransparentObjects) {
                QSSGRenderableObject *theObject = handle.obj;
                if (!(theObject->renderableFlags.isCompletelyTransparent())) {
                    QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
                    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());
                    inRenderFn(*this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
                }
            }
        }
    }
}

void QSSGLayerRenderData::render(QSSGResourceFrameBuffer *theFB)
{
    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera == nullptr)
        return;

    renderer->beginLayerRender(*this);
    runRenderPass(renderRenderable, true, !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass), false, true, 0, *camera, theFB);
    renderer->endLayerRender();
}

void QSSGLayerRenderData::createGpuProfiler()
{
    if (renderer->context()->supportsTimerQuery()) {
        m_layerProfilerGpu.reset(new QSSGRenderGPUProfiler(renderer->contextInterface(), renderer->context()));
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

static inline void offsetProjectionMatrix(QMatrix4x4 &inProjectionMatrix,
                                          const QVector2D &inVertexOffsets)
{
    inProjectionMatrix(0, 3) += inProjectionMatrix(3, 3) * inVertexOffsets.x();
    inProjectionMatrix(1, 3) += inProjectionMatrix(3, 3) * inVertexOffsets.y();
}

void QSSGLayerRenderData::applyLayerPostEffects(const QSSGRef<QSSGRenderFrameBuffer> &theFB)
{
    if (layer.firstEffect == nullptr || camera == nullptr)
        return;

    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    const auto lastEffect = thePrepResult.lastEffect;
    // we use the non MSAA buffer for the effect
    const QSSGRef<QSSGRenderTexture2D> &theLayerColorTexture = m_layerTexture.getTexture();
    const QSSGRef<QSSGRenderTexture2D> &theLayerDepthTexture = m_layerDepthTexture.getTexture();

    QSSGRef<QSSGRenderTexture2D> theCurrentTexture = theLayerColorTexture;
    const QSSGRef<QSSGEffectSystem> &theEffectSystem(renderer->contextInterface()->effectSystem());
    const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->contextInterface()->resourceManager());

    // Process all effect except the last one as the last effect should target the original FB
    for (QSSGRenderEffect *theEffect = layer.firstEffect; theEffect && theEffect != lastEffect; theEffect = theEffect->m_nextEffect) {
        if (theEffect->flags.testFlag(QSSGRenderEffect::Flag::Active)) {
            startProfiling(theEffect->className, false);
            QSSGRef<QSSGRenderTexture2D> theRenderedEffect = theEffectSystem->renderEffect(QSSGEffectRenderArgument(theEffect,
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

            if (Q_UNLIKELY(!theRenderedEffect)) {
                QString errorMsg = QObject::tr("Failed to compile \"%1\" effect.\nConsider"
                                               " removing it from the presentation.")
                                           .arg(QString::fromLatin1(theEffect->className));
                qFatal("%s", errorMsg.toUtf8().constData());
            }
        }
    }

    // Last Effect should render directly to theFB
    // If there is a last effect, it has already been confirmed to be active
    if (layerPrepResult->lastEffect) {
        const auto &theContext = renderer->context();
        theContext->setRenderTarget(theFB);
        theContext->setViewport(layerPrepResult->viewport().toRect());
        theContext->setScissorTestEnabled(true);
        theContext->setScissorRect(layerPrepResult->scissor().toRect());
        const QSSGRef<QSSGEffectSystem> &theEffectSystem(renderer->contextInterface()->effectSystem());
        startProfiling(lastEffect->className, false);
        QMatrix4x4 theMVP;
        QSSGRenderCamera::setupOrthographicCameraForOffscreenRender(*theCurrentTexture, theMVP);
        theEffectSystem->renderEffect(QSSGEffectRenderArgument(lastEffect,
                                                               theCurrentTexture,
                                                               QVector2D(camera->clipNear, camera->clipFar),
                                                               theLayerDepthTexture,
                                                               m_layerPrepassDepthTexture),
                                      theMVP,
                                      false);

        endProfiling(lastEffect->className);
    }
}

inline bool anyCompletelyNonTransparentObjects(const QSSGLayerRenderPreparationData::TRenderableObjectList &inObjects)
{
    for (int idx = 0, end = inObjects.size(); idx < end; ++idx) {
        if (!inObjects.at(idx).obj->renderableFlags.isCompletelyTransparent())
            return true;
    }
    return false;
}

bool QSSGLayerRenderData::progressiveAARenderRequest() const
{
    const QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    return m_progressiveAAPassIndex && m_progressiveAAPassIndex < thePrepResult.maxAAPassIndex;
}

void QSSGLayerRenderData::runnableRenderToViewport(const QSSGRef<QSSGRenderFrameBuffer> &theFB)
{
    const auto &theContext = renderer->context();
    theContext->resetStates();

    QSSGRenderContextScopedProperty<const QSSGRef<QSSGRenderFrameBuffer> &> __fbo(*theContext,
                                                                                &QSSGRenderContext::renderTarget,
                                                                                &QSSGRenderContext::setRenderTarget);
    QSSGRenderContextScopedProperty<QRect> __viewport(*theContext, &QSSGRenderContext::viewport, &QSSGRenderContext::setViewport);
    QSSGRenderContextScopedProperty<bool> theScissorEnabled(*theContext,
                                                              &QSSGRenderContext::isScissorTestEnabled,
                                                              &QSSGRenderContext::setScissorTestEnabled);
    QSSGRenderContextScopedProperty<QRect> theScissorRect(*theContext,
                                                            &QSSGRenderContext::scissorRect,
                                                            &QSSGRenderContext::setScissorRect);
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    QRectF theScreenRect(thePrepResult.viewport());

    const bool isProgressiveAABlendPass = m_progressiveAAPassIndex
                    && m_progressiveAAPassIndex < thePrepResult.maxAAPassIndex;
    const bool isProgressiveAACopyPass = !isProgressiveAABlendPass
                    && layer.antialiasingMode == QSSGRenderLayer::AAMode::ProgressiveAA;
    const bool isTemporalAABlendPass = layer.temporalAAEnabled
                    && !qFuzzyIsNull(layer.temporalAAStrength);
    const bool isTemporalNoProgressiveBlend = isTemporalAABlendPass && !isProgressiveAABlendPass;
    quint32 aaFactorIndex = 0;

    // here used only for temporal aa
    QSSGRef<QSSGLayerProgAABlendShader> temporalAABlendShader = nullptr;

    // progressive aa uses this one
    QSSGRef<QSSGLayerLastFrameBlendShader> progAABlendShader = nullptr;

    // Composit shader used by the post-processing effect stage
    QSSGRef<QSSGCompositShader> compositShader = nullptr;

    qint32 sampleCount = 1;
    // check multsample mode and MSAA texture support
    if (layer.antialiasingMode == QSSGRenderLayer::AAMode::MSAA && theContext->supportsMultisampleTextures())
        sampleCount = qint32(layer.antialiasingQuality);

    if (isTemporalAABlendPass || isProgressiveAABlendPass || isProgressiveAACopyPass) {
        if (isTemporalAABlendPass)
            temporalAABlendShader = renderer->getLayerProgAABlendShader();
        if (isProgressiveAABlendPass)
            progAABlendShader = renderer->getLayerLastFrameBlendShader();

        // we use the temporal aa texture for progressive aa too
        m_temporalAATexture.ensureTexture(theScreenRect.width(), theScreenRect.height(),
                                          QSSGRenderTextureFormat::RGBA8);

        if ((!isProgressiveAACopyPass || isTemporalNoProgressiveBlend) && sampleCount <= 1) {
            // Note: TemporalAA doesn't work together with multisampling
            QVector2D theVertexOffsets;
            if (isProgressiveAABlendPass) {
                aaFactorIndex = (m_progressiveAAPassIndex - 1);
                theVertexOffsets = s_VertexOffsets[aaFactorIndex];
            } else {
                const float temporalStrength = layer.temporalAAStrength;
                const QVector2D s_TemporalVertexOffsets[QSSGLayerRenderPreparationData::MAX_TEMPORAL_AA_LEVELS] = {
                    QVector2D(temporalStrength, temporalStrength),
                    QVector2D(-temporalStrength, -temporalStrength)
                };
                theVertexOffsets = s_TemporalVertexOffsets[m_temporalAAPassIndex];
                if (layer.antialiasingMode == QSSGRenderLayer::AAMode::SSAA) {
                    // temporal offset needs to grow with SSAA resolution
                    theVertexOffsets *= layer.ssaaMultiplier;
                }
                ++m_temporalAAPassIndex;
                m_temporalAAPassIndex = m_temporalAAPassIndex % MAX_TEMPORAL_AA_LEVELS;
            }

            theVertexOffsets.setX(theVertexOffsets.x() / (theScreenRect.width() / 2.0f));
            theVertexOffsets.setY(theVertexOffsets.y() / (theScreenRect.height() / 2.0f));
            // Run through all models and update MVP.

            // TODO - optimize this exact matrix operation.
            for (qint32 idx = 0, end = modelContexts.size(); idx < end; ++idx) {
                QMatrix4x4 &originalProjection(modelContexts[idx]->modelViewProjection);
                offsetProjectionMatrix(originalProjection, theVertexOffsets);
            }
        }
    }

    // Shadows and SSAO require an FBO, so create one if we are using those
    if (thePrepResult.flags.requiresSsaoPass() || thePrepResult.flags.requiresShadowMapPass()) {
        QSize theLayerTextureDimensions = thePrepResult.textureDimensions();
        QSSGRef<QSSGResourceManager> theResourceManager = renderer->contextInterface()->resourceManager();
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

    QSSGResourceFrameBuffer thePreFBO(nullptr);
    const bool hasPostProcessingEffects = (thePrepResult.lastEffect != nullptr); /* we have effects */
    if (hasPostProcessingEffects) {
        QSize theLayerTextureDimensions = thePrepResult.textureDimensions();
        QSSGRef<QSSGResourceManager> theResourceManager = renderer->contextInterface()->resourceManager();
        thePreFBO = theResourceManager;
        // Allocates the frame buffer which has the side effect of setting the current render target
        // to that frame buffer.
        thePreFBO.ensureFrameBuffer();
        theContext->setScissorTestEnabled(false);
        // Setup the default render target type
        QSSGRenderTextureFormat outputFormat = QSSGRenderTextureFormat::RGBA8;
        if (theContext->supportsFpRenderTarget()) {
            if (theContext->renderContextType() == QSSGRenderContextType::GL3 ||
                theContext->renderContextType() == QSSGRenderContextType::GL4)
                outputFormat = QSSGRenderTextureFormat::RGBA32F;
            else
                outputFormat = QSSGRenderTextureFormat::RGBA16F;
        }

        if (m_layerTexture.ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), outputFormat)) {
            m_layerTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            m_layerTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
        }

        if (m_layerDepthTexture.ensureTexture(theLayerTextureDimensions.width(), theLayerTextureDimensions.height(), QSSGRenderTextureFormat::Depth24Stencil8)) {
            // Depth textures are generally not bilinear filtered.
            m_layerDepthTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Nearest);
            m_layerDepthTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Nearest);
        }

        // Setup FBO with single color buffer target
        thePreFBO->attach(QSSGRenderFrameBufferAttachment::Color0, m_layerTexture.getTexture());
        QSSGRenderFrameBufferAttachment theAttachment = getFramebufferDepthAttachmentFormat(QSSGRenderTextureFormat::Depth24Stencil8);
        thePreFBO->attach(theAttachment, m_layerDepthTexture.getTexture());
        theContext->setRenderTarget(thePreFBO);
    } else {
        theContext->setRenderTarget(theFB);
    }

    // Multisampling
    theContext->setMultisampleEnabled(sampleCount > 1);

    // Start Operations on Viewport
    theContext->setViewport(layerPrepResult->viewport().toRect());
    theContext->setScissorTestEnabled(true);
    theContext->setScissorRect(layerPrepResult->scissor().toRect());

    // Depth Pre-pass
    if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)) {
        startProfiling("Depth pass", false);
        renderDepthPass(false);
        endProfiling("Depth pass");
    }

    // Viewport Clear
    startProfiling("Clear pass", false);
    renderClearPass();
    endProfiling("Clear pass");

    // Render pass
    startProfiling("Render pass", false);
    render();
    endProfiling("Render pass");

#ifdef QT_QUICK3D_DEBUG_SHADOWS
    if (shadowMapManager->getShadowMapEntry(0)->m_depthMap) {
        renderDebugDepthMap(shadowMapManager->getShadowMapEntry(0)->m_depthMap.get(),
                            shadowMapManager->getShadowMapEntry(0)->m_depthCube.get());
    }
#endif

    if (hasPostProcessingEffects)
        applyLayerPostEffects(theFB);

    if (temporalAABlendShader && isTemporalNoProgressiveBlend && sampleCount <= 1) {
        // Note: TemporalAA doesn't work together with multisampling
        theContext->copyFramebufferTexture(0, 0, theScreenRect.width(), theScreenRect.height(),
                                           0, 0,
                                           QSSGRenderTextureOrRenderBuffer(m_temporalAATexture));

        if (m_prevTemporalAATexture.isNull()) {
            // If m_prevTemporalAATexture doesn't exist yet, copy current to avoid flicker
            m_prevTemporalAATexture.ensureTexture(theScreenRect.width(), theScreenRect.height(),
                                                  QSSGRenderTextureFormat::RGBA8);
            theContext->copyFramebufferTexture(0, 0, theScreenRect.width(), theScreenRect.height(),
                                               0, 0,
                                               QSSGRenderTextureOrRenderBuffer(m_prevTemporalAATexture));
        }
        // blend temporal aa textures
        QVector2D theBlendFactors;
        theBlendFactors = QVector2D(.5f, .5f);

        theContext->setDepthTestEnabled(false);
        theContext->setBlendingEnabled(false);
        theContext->setCullingEnabled(false);
        theContext->setActiveShader(temporalAABlendShader->shader);
        temporalAABlendShader->accumSampler.set(m_prevTemporalAATexture.getTexture().data());
        temporalAABlendShader->lastFrame.set(m_temporalAATexture.getTexture().data());
        temporalAABlendShader->blendFactors.set(theBlendFactors);
        renderer->renderQuad();
        m_prevTemporalAATexture.swapTexture(m_temporalAATexture);
    }
    if (isProgressiveAACopyPass || (progAABlendShader && isProgressiveAABlendPass)) {
        // first pass is just copying the frame, next passes blend the texture
        // on top of the screen
        if (m_progressiveAAPassIndex > 1 && progAABlendShader) {
            theContext->setDepthTestEnabled(false);
            theContext->setBlendingEnabled(true);
            theContext->setCullingEnabled(false);
            theContext->setBlendFunction(QSSGRenderBlendFunctionArgument(
                                             QSSGRenderSrcBlendFunc::One, QSSGRenderDstBlendFunc::OneMinusSrcAlpha,
                                             QSSGRenderSrcBlendFunc::Zero, QSSGRenderDstBlendFunc::One));
            const float blendFactor = s_BlendFactors[aaFactorIndex].y();
            theContext->setActiveShader(progAABlendShader->shader);
            progAABlendShader->lastFrame.set(m_temporalAATexture.getTexture().data());
            progAABlendShader->blendFactor.set(blendFactor);
            renderer->renderQuad();
        }
        theContext->copyFramebufferTexture(0, 0, theScreenRect.width(), theScreenRect.height(),
                                           0, 0,
                                           QSSGRenderTextureOrRenderBuffer(m_temporalAATexture));
        if (m_progressiveAAPassIndex < thePrepResult.maxAAPassIndex)
            ++m_progressiveAAPassIndex;
    }
}

void QSSGLayerRenderData::prepareForRender()
{
    // When we render to the scene itself (as opposed to an offscreen buffer somewhere)
    // then we use the MVP of the layer somewhat.
    const QSize theViewportSize = renderer->contextInterface()->viewport().size();
    prepareForRender(theViewportSize);
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

QT_END_NAMESPACE
