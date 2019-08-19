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

#ifndef QSSG_RENDER_PATH_RENDER_CONTEXT_H
#define QSSG_RENDER_PATH_RENDER_CONTEXT_H

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

#include <QtQuick3DUtils/private/qssgbounds3_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpath_p.h>

#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE

struct QSSGPathRenderContext
{
    // The lights and camera will not change per layer,
    // so that information can be set once for all the shaders.
    const QVector<QSSGRenderLight *> &lights;
    const QSSGRenderCamera &camera;

    // Per-object information.
    const QSSGRenderPath &path;
    const QMatrix4x4 &mvp;
    const QMatrix4x4 &modelMatrix; ///< model to world transformation
    const QMatrix3x3 &normalMatrix;

    float opacity;
    const QSSGRenderGraphObject &material;
    QSSGShaderDefaultMaterialKey materialKey;
    QSSGRenderableImage *firstImage;
    QVector2D cameraVec;

    bool enableWireframe;
    bool hasTransparency;
    bool isStroke;

    QSSGPathRenderContext(const QVector<QSSGRenderLight *> &inLights,
                            const QSSGRenderCamera &inCamera,
                            const QSSGRenderPath &inPath,
                            const QMatrix4x4 &inMvp,
                            const QMatrix4x4 &inWorld,
                            const QMatrix3x3 &inNormal,
                            float inOpacity,
                            const QSSGRenderGraphObject &inMaterial,
                            QSSGShaderDefaultMaterialKey inMaterialKey,
                            QSSGRenderableImage *inFirstImage,
                            bool inWireframe,
                            QVector2D inCameraVec,
                            bool inHasTransparency,
                            bool inIsStroke)

        : lights(inLights)
        , camera(inCamera)
        , path(inPath)
        , mvp(inMvp)
        , modelMatrix(inWorld)
        , normalMatrix(inNormal)
        , opacity(inOpacity)
        , material(inMaterial)
        , materialKey(inMaterialKey)
        , firstImage(inFirstImage)
        , cameraVec(inCameraVec)
        , enableWireframe(inWireframe)
        , hasTransparency(inHasTransparency)
        , isStroke(inIsStroke)
    {
    }
};
QT_END_NAMESPACE
#endif
