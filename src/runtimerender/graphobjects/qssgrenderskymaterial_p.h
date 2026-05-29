// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSG_RENDER_SKY_MATERIAL_H
#define QSSG_RENDER_SKY_MATERIAL_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderhelpers_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiEffectSystem;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderSkyMaterial : public QSSGRenderGraphObject
{
    int radianceMapSize = 512;
    int iblSampleCount = 16;
    int iblSamplesPerFrame = 0;
    bool wantsMoreFrames = false;
    bool enableIBL = false;

    QSSGRenderSkyMaterial();
    ~QSSGRenderSkyMaterial();

    QList<QSSGBaseTypeProperty> propertyUniforms;

    QSSGRhiShaderPipelinePtr iblPassPipeline;

    QSSGRhiShaderPipelinePtr ensurePipeline(const QSSGRenderContextInterface &sgContext);

    // returns uniform stride per face
    quint32 updateUniforms(const QSSGRenderContextInterface &sgContext, const QMatrix4x4 &mvp, const QVarLengthArray<QMatrix4x4, 6> views);

    QByteArray fragmentShaderSource;

    QByteArray shaderPathKey = "sky material --";

    bool isDirty = true;
    bool isFragmentShaderDirty = true;

    QSSGRhiShaderResourceBindingList bindings;
};

QT_END_NAMESPACE

#endif // QSSG_RENDER_SKY_MATERIAL_H
