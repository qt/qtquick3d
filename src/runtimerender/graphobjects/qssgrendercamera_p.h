// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_CAMERA_H
#define QSSG_RENDER_CAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderray_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

struct QSSGCameraGlobalCalculationResult
{
    bool m_wasDirty;
    bool m_computeFrustumSucceeded /* = true */;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderCamera : public QSSGRenderNode
{
    enum class DirtyFlag : quint8
    {
        CameraDirty = 0x1
    };
    using FlagT = std::underlying_type_t<DirtyFlag>;

    static constexpr DirtyFlag DirtyMask { std::numeric_limits<FlagT>::max() };

    // Setting these variables should set dirty on the camera.
    float clipNear;
    float clipFar;

    float fov; // Radians
    bool fovHorizontal;

    float top = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;
    float right = 0.0f;

    float horizontalMagnification = 1.0f;
    float verticalMagnification = 1.0f;

    float dpr = 1.0f;

    QMatrix4x4 projection;
    // Record some values from creating the projection matrix
    // to use during mouse picking.
    QVector2D frustumScale;
    bool enableFrustumClipping;
    FlagT cameraDirtyFlags = 0;

    float levelOfDetailPixelThreshold = 1.0;

    QRectF previousInViewport;

    explicit QSSGRenderCamera(QSSGRenderGraphObject::Type type);

    QMatrix3x3 getLookAtMatrix(const QVector3D &inUpDir, const QVector3D &inDirection) const;
    // Set our position, rotation member variables based on the lookat target
    // Marks this object as dirty.
    // Need to test this when the camera's local transform is null.
    // Assumes parent's local transform is the identity, meaning our local transform is
    // our global transform.
    void lookAt(const QVector3D &inCameraPos, const QVector3D &inUpDir, const QVector3D &inTargetPos, const QVector3D &pivot);

    QSSGCameraGlobalCalculationResult calculateGlobalVariables(const QRectF &inViewport);
    bool calculateProjection(const QRectF &inViewport);
    bool computeFrustumOrtho(const QRectF &inViewport);
    // Used when rendering the widgets in studio.  This scales the widget when in orthographic
    // mode in order to have
    // constant size on screen regardless.
    // Number is always greater than one
    float getOrthographicScaleFactor(const QRectF &inViewport) const;
    bool computeFrustumPerspective(const QRectF &inViewport);
    bool computeCustomFrustum(const QRectF &inViewport);

    static void calculateViewProjectionMatrix(const QMatrix4x4 &globalTransform,
                                              const QMatrix4x4 &projection,
                                              QMatrix4x4 &outMatrix);

    void calculateViewProjectionMatrix(QMatrix4x4 &outMatrix) const;
    void calculateViewProjectionMatrix(QMatrix4x4 &outMatrix, float clipNear, float clipFar) const;

    void calculateViewProjectionWithoutTranslation(float near, float far, QMatrix4x4 &outMatrix) const;

    // Unproject a point (x,y) in viewport relative coordinates meaning
    // left, bottom is 0,0 and values are increasing right,up respectively.
    QSSGRenderRay unproject(const QVector2D &inLayerRelativeMouseCoords, const QRectF &inViewport) const;

    // Unproject a given coordinate to a 3d position that lies on the same camera
    // plane as inGlobalPos.
    // Expects CalculateGlobalVariables has been called or doesn't need to be.
    QVector3D unprojectToPosition(const QVector3D &inGlobalPos, const QSSGRenderRay &inRay) const;

    float verticalFov(float aspectRatio) const;
    float verticalFov(const QRectF &inViewport) const;

    [[nodiscard]] inline bool isDirty(DirtyFlag dirtyFlag = DirtyMask) const
    {
        return ((cameraDirtyFlags & FlagT(dirtyFlag)) != 0)
               || ((dirtyFlag == DirtyMask) && QSSGRenderNode::isDirty());
    }
    void markDirty(DirtyFlag dirtyFlag);
    void clearDirty(DirtyFlag dirtyFlag);

    float getLevelOfDetailMultiplier() const;
};

QT_END_NAMESPACE

#endif
