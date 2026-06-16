// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGCOMPUTEMANAGER_P_H
#define QSSGCOMPUTEMANAGER_P_H

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

#include <QtQuick3DRuntimeRender/qtquick3druntimerenderglobal.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContextInterface;
class QSSGRenderComputePass;

struct ComputeUserPassData
{
    QSSGRenderShaderValue::ivec3 m_groupCount = {0, 0, 0};
    QSSGRhiShaderPipelinePtr m_computePipeline = nullptr;
    QRhiShaderResourceBindings *m_computeBindings = nullptr;
    QSSGShaderDefineList m_shaderDefines;
};

bool finalizeComputeShader(QSSGUserComputeShader &computeShader);
void prepareComputePass(QSSGRenderer &renderer, QSSGLayerRenderData &data, QSSGRenderComputePass *passNode, ComputeUserPassData &passData, QSSGPassKey key);

QT_END_NAMESPACE

#endif
