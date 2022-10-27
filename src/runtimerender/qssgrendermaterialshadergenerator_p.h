// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_MATERIAL_SHADER_GENERATOR_H
#define QSSG_RENDER_MATERIAL_SHADER_GENERATOR_H

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
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtCore/qvector.h>
#include <QtGui/QVector3D>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>


QT_BEGIN_NAMESPACE

struct QSSGRenderLayer;
struct QSSGRenderCamera;
struct QSSGRenderLight;
class QSSGRenderShadowMap;
struct QSSGRenderImage;
class QRhiTexture;
struct QSSGCameraRenderData;

struct QSSGLayerGlobalRenderProperties
{
    const QSSGRenderLayer &layer;
    const QSSGRenderCamera &camera;
    const QSSGCameraRenderData &cameraData;
    QSSGRenderShadowMap *shadowMapManager;
    QRhiTexture *rhiDepthTexture;
    QRhiTexture *rhiSsaoTexture;
    QRhiTexture *rhiScreenTexture;
    QSSGRenderImage *lightProbe;
    float probeHorizon;
    float probeExposure;
    const QMatrix3x3 &probeOrientation;
    bool isYUpInFramebuffer;
    bool isYUpInNDC;
    bool isClipDepthZeroToOne;
};

QT_END_NAMESPACE

#endif
