/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

struct QSSGLayerGlobalRenderProperties
{
    const QSSGRenderLayer &layer;
    QSSGRenderCamera &camera;
    QVector3D cameraDirection;
    QSSGRenderShadowMap *shadowMapManager;
    QRhiTexture *rhiDepthTexture;
    QRhiTexture *rhiSsaoTexture;
    QRhiTexture *rhiScreenTexture;
    QSSGRenderImage *lightProbe;
    float probeHorizon;
    float probeExposure;
    const QMatrix4x4 &probeOrientation;
    bool isYUpInFramebuffer;
    bool isYUpInNDC;
    bool isClipDepthZeroToOne;
};

QT_END_NAMESPACE

#endif
