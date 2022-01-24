/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QSSG_RENDER_REFLECTION_MAP_H
#define QSSG_RENDER_REFLECTION_MAP_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderreflectionprobe_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRenderContextInterface;

class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiTexture;
class QRhiGraphicsPipeline;
class QRhiShaderResourceBindings;
class QRhiBuffer;

struct QSSGReflectionMapEntry
{
    QSSGReflectionMapEntry();

    static QSSGReflectionMapEntry withRhiCubeMap(quint32 probeIdx,
                                                 QRhiTexture *cube,
                                                 QRhiTexture *prefiltered,
                                                 QRhiRenderBuffer *depthStencil);

    void renderMips(QSSGRhiContext *context);
    void destroyRhiResources();

    quint32 m_probeIndex;

    // RHI resources
    QRhiTexture *m_rhiCube = nullptr;
    QRhiTexture *m_rhiPrefilteredCube = nullptr;
    QRhiRenderBuffer *m_rhiDepthStencil = nullptr;
    QVarLengthArray<QRhiTextureRenderTarget *, 6> m_rhiRenderTargets;
    QRhiRenderPassDescriptor *m_rhiRenderPassDesc = nullptr;

    QRhiGraphicsPipeline *m_prefilterPipeline = nullptr;
    QRhiGraphicsPipeline *m_irradiancePipeline = nullptr;
    QRhiShaderResourceBindings *m_prefilterSrb = nullptr;
    QRhiShaderResourceBindings *m_irradianceSrb = nullptr;
    QRhiBuffer *m_prefilterVertBuffer = nullptr;
    QRhiBuffer *m_prefilterFragBuffer = nullptr;
    QRhiBuffer *m_irradianceFragBuffer = nullptr;
    QMap<int, QVarLengthArray<QRhiTextureRenderTarget *, 6>> m_rhiPrefilterRenderTargetsMap;
    QRhiRenderPassDescriptor *m_rhiPrefilterRenderPassDesc = nullptr;
    QMap<int, QSize> m_prefilterMipLevelSizes;

    QVarLengthArray<QRhiShaderResourceBindings *, 6> m_skyBoxSrbs;

    QMatrix4x4 m_viewProjection;

    bool m_needsRender = false;
    bool m_rendered = false;

    QSSGRenderReflectionProbe::ReflectionTimeSlicing m_timeSlicing = QSSGRenderReflectionProbe::ReflectionTimeSlicing::None;
    int m_timeSliceFrame = 1;
    int m_timeSliceFace = 0;
};

class QSSGRenderReflectionMap
{
    typedef QVector<QSSGReflectionMapEntry> TReflectionMapEntryList;

public:
    QAtomicInt ref;
    const QSSGRenderContextInterface &m_context;

    QSSGRenderReflectionMap(const QSSGRenderContextInterface &inContext);
    ~QSSGRenderReflectionMap();

    void addReflectionMapEntry(qint32 probeIdx, const QSSGRenderReflectionProbe &probe);

    QSSGReflectionMapEntry *reflectionMapEntry(int probeIdx);

    qint32 reflectionMapEntryCount() { return m_reflectionMapList.size(); }

private:
    TReflectionMapEntryList m_reflectionMapList;
};

QT_END_NAMESPACE

#endif
