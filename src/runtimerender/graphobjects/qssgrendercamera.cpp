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


#include "qssgrendercamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtGui/QVector2D>

#include <qmath.h>

QT_BEGIN_NAMESPACE

namespace {

float getAspectRatio(const QRectF &inViewport)
{
    return inViewport.height() != 0 ? inViewport.width() / inViewport.height() : 0.0f;
}

}

QSSGRenderCamera::QSSGRenderCamera()
    : QSSGRenderNode(QSSGRenderGraphObject::Type::Camera)
    , clipNear(10)
    , clipFar(10000)
    , fov(qDegreesToRadians(60.0f))
    , fovHorizontal(false)
    , enableFrustumClipping(true)
{
    projection = QMatrix4x4();
    position = QVector3D(0, 0, 600);

    flags.setFlag(Flag::CameraDirty, true);
}

// Code for testing
QSSGCameraGlobalCalculationResult QSSGRenderCamera::calculateGlobalVariables(const QRectF &inViewport)
{
    bool wasDirty = QSSGRenderNode::calculateGlobalVariables();
    return QSSGCameraGlobalCalculationResult{ wasDirty, calculateProjection(inViewport) };
}

bool QSSGRenderCamera::calculateProjection(const QRectF &inViewport)
{
    bool retval = false;

    const bool argumentsChanged = inViewport != previousInViewport;
    if (!argumentsChanged && !flags.testFlag(Flag::CameraDirty))
        return true;
    previousInViewport = inViewport;
    flags.setFlag(Flag::CameraDirty, false);

    if (flags.testFlag(Flag::CameraCustomProjection))
        retval = true; // AKA, do nothing
    else if (flags.testFlag(Flag::CameraFrustumProjection))
        retval = computeCustomFrustum(inViewport);
    else if (flags.testFlag(Flag::Orthographic))
        retval = computeFrustumOrtho(inViewport);
    else
        retval = computeFrustumPerspective(inViewport);
    if (retval) {
        float *writePtr(projection.data());
        frustumScale.setX(writePtr[0]);
        frustumScale.setY(writePtr[5]);
    }
    return retval;
}

//==============================================================================
/**
 *	Compute the projection matrix for a perspective camera
 *	@return true if the computed projection matrix is valid
 */
bool QSSGRenderCamera::computeFrustumPerspective(const QRectF &inViewport)
{
    projection = QMatrix4x4();
    projection.perspective(qRadiansToDegrees(verticalFov(inViewport)), getAspectRatio(inViewport), clipNear, clipFar);
    return true;
}

bool QSSGRenderCamera::computeCustomFrustum(const QRectF &inViewport)
{
    Q_UNUSED(inViewport)
    projection.setToIdentity();
    projection.frustum(left, right, bottom, top, clipNear, clipFar);
    return true;
}

//==============================================================================
/**
 *	Compute the projection matrix for a orthographic camera
 *	@return true if the computed projection matrix is valid
 */
bool QSSGRenderCamera::computeFrustumOrtho(const QRectF &inViewport)
{
    projection = QMatrix4x4();
    float halfWidth = inViewport.width() / 2.0f;
    float halfHeight = inViewport.height() / 2.0f;
    projection.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, clipNear, clipFar);
    return true;
}

float QSSGRenderCamera::getOrthographicScaleFactor(const QRectF &inViewport) const
{
    Q_UNUSED(inViewport);
    return 1.0f;
}

static QQuaternion rotationQuaternionForLookAt(const QVector3D &sourcePosition,
                                               const QVector3D &sourceDirection,
                                               const QVector3D &targetPosition,
                                               const QVector3D &upDirection)
{
    QVector3D targetDirection = sourcePosition - targetPosition;
    targetDirection.normalize();

    QVector3D rotationAxis = QVector3D::crossProduct(sourceDirection, targetDirection);

    const QVector3D normalizedAxis = rotationAxis.normalized();
    if (qFuzzyIsNull(normalizedAxis.lengthSquared()))
        rotationAxis = upDirection;

    float dot = QVector3D::dotProduct(sourceDirection, targetDirection);
    float rotationAngle = float(qRadiansToDegrees(qAcos(qreal(dot))));

    return QQuaternion::fromAxisAndAngle(rotationAxis, rotationAngle);
}

void QSSGRenderCamera::lookAt(const QVector3D &inCameraPos, const QVector3D &inUpDir, const QVector3D &inTargetPos)
{
    rotation = rotationQuaternionForLookAt(inCameraPos, getScalingCorrectDirection(), inTargetPos, inUpDir.normalized());
    position = inCameraPos;
    markDirty(TransformDirtyFlag::TransformIsDirty);
}

void QSSGRenderCamera::calculateViewProjectionMatrix(QMatrix4x4 &outMatrix) const
{
    outMatrix = projection * globalTransform.inverted();
}

QSSGCuboidRect QSSGRenderCamera::getCameraBounds(const QRectF &inViewport) const
{
    Q_UNUSED(inViewport)
    QSSGCuboidRect normalizedCuboid(-1, 1, 1, -1);
    return normalizedCuboid;
}

void QSSGRenderCamera::setupOrthographicCameraForOffscreenRender(QSSGRenderTexture2D &inTexture, QMatrix4x4 &outVP)
{
    QSSGTextureDetails theDetails(inTexture.textureDetails());
    QSSGRenderCamera theTempCamera;
    theTempCamera.flags.setFlag(Flag::Orthographic);
    theTempCamera.markDirty(TransformDirtyFlag::TransformIsDirty);
    theTempCamera.calculateGlobalVariables(QRect(0, 0, theDetails.width, theDetails.height));
    theTempCamera.calculateViewProjectionMatrix(outVP);
}

QSSGRenderRay QSSGRenderCamera::unproject(const QVector2D &inViewportRelativeCoords,
                                              const QRectF &inViewport) const
{
    QSSGRenderRay theRay;
    QVector2D globalCoords = toAbsoluteCoords(inViewport, inViewportRelativeCoords);
    QVector2D normalizedCoords = absoluteToNormalizedCoordinates(inViewport, globalCoords);
    QVector3D &outOrigin(theRay.origin);
    QVector3D &outDir(theRay.direction);
    QVector2D inverseFrustumScale(1.0f / frustumScale.x(), 1.0f / frustumScale.y());
    QVector2D scaledCoords(inverseFrustumScale.x() * normalizedCoords.x(), inverseFrustumScale.y() * normalizedCoords.y());

    if (flags.testFlag(Flag::Orthographic)) {
        outOrigin.setX(scaledCoords.x());
        outOrigin.setY(scaledCoords.y());
        outOrigin.setZ(0.0f);

        outDir.setX(0.0f);
        outDir.setY(0.0f);
        outDir.setZ(-1.0f);
    } else {
        outOrigin.setX(0.0f);
        outOrigin.setY(0.0f);
        outOrigin.setZ(0.0f);

        outDir.setX(scaledCoords.x());
        outDir.setY(scaledCoords.y());
        outDir.setZ(-1.0f);
    }

    outOrigin = mat44::transform(globalTransform, outOrigin);
    QMatrix3x3 theNormalMatrix = calculateNormalMatrix();

    outDir = mat33::transform(theNormalMatrix, outDir);
    outDir.normalize();

    return theRay;
}

QVector3D QSSGRenderCamera::unprojectToPosition(const QVector3D &inGlobalPos, const QSSGRenderRay &inRay) const
{
    QVector3D theCameraDir = getDirection();
    QVector3D theObjGlobalPos = inGlobalPos;
    float theDistance = -1.0f * QVector3D::dotProduct(theObjGlobalPos, theCameraDir);
    QSSGPlane theCameraPlane(theCameraDir, theDistance);
    return QSSGRenderRay::intersect(theCameraPlane, inRay);
}

float QSSGRenderCamera::verticalFov(float aspectRatio) const
{
    return fovHorizontal ? float(2.0 * qAtan(qTan(qreal(fov) / 2.0) / qreal(aspectRatio))) : fov;
}

float QSSGRenderCamera::verticalFov(const QRectF &inViewport) const
{
    return verticalFov(getAspectRatio(inViewport));
}

QT_END_NAMESPACE
