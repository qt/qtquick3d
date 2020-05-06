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
    , m_depthBufferFormat(QSSGRenderTextureFormat::Unknown)
    , m_progressiveAAPassIndex(0)
    , m_temporalAAPassIndex(0)
    , m_textScale(1.0f)
    , m_zPrePassPossible(true)
{
}

QSSGLayerRenderData::~QSSGLayerRenderData()
{
    m_rhiDepthTexture.reset();
    m_rhiAoTexture.reset();
}

void QSSGLayerRenderData::prepareForRender(const QSize &inViewportDimensions)
{
    QSSGLayerRenderPreparationData::prepareForRender(inViewportDimensions);
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);
    const QSSGRef<QSSGResourceManager> &theResourceManager(renderer->contextInterface()->resourceManager());

    // Generate all necessary lighting keys

    if (thePrepResult.flags.wasLayerDataDirty()) {
        m_progressiveAAPassIndex = 0;
    }

    renderer->layerNeedsFrameClear(*this);

    // Clean up the texture cache if layer dimensions changed
    if (inViewportDimensions.width() != m_previousDimensions.width()
            || inViewportDimensions.height() != m_previousDimensions.height()) {

        m_previousDimensions.setWidth(inViewportDimensions.width());
        m_previousDimensions.setHeight(inViewportDimensions.height());

        theResourceManager->destroyFreeSizedResources();

    }
}

namespace RendererImpl {

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

void setupCameraForShadowMap(const QRectF &inViewport,
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

} // namespace RendererImpl

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
    return false; // This is all done through requestedFramesCount now
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
    m_zPrePassPossible = true;
}

QT_END_NAMESPACE
