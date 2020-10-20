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

#ifndef QSSG_RENDER_CUSTOM_MATERIAL_H
#define QSSG_RENDER_CUSTOM_MATERIAL_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlightmaps_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvector.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

struct QSSGShaderMaterialAdapter;
class QQuick3DShaderUtilsTextureInput;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderCustomMaterial : public QSSGRenderGraphObject
{
    QSSGRenderCustomMaterial();
    ~QSSGRenderCustomMaterial();

    struct TextureProperty
    {
        QQuick3DShaderUtilsTextureInput *texInput = nullptr;
        QSSGRenderImage *texImage = nullptr;
        QByteArray name;
        QSSGRenderShaderDataType shaderDataType;
        QSSGRenderTextureFilterOp minFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureFilterOp magFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureFilterOp mipFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureCoordOp clampType = QSSGRenderTextureCoordOp::ClampToEdge;
    };

    struct Property
    {
        Property() = default;
        Property(const QByteArray &name, const QVariant &value, QSSGRenderShaderDataType shaderDataType, int pid = -1)
            : name(name), value(value), shaderDataType(shaderDataType), pid(pid)
        { }
        QByteArray name;
        QVariant value;
        QSSGRenderShaderDataType shaderDataType;
        int pid;
    };

    enum class ShadingMode // must match QQuick3DCustomMaterial::ShadingMode
    {
        Unshaded,
        Shaded
    };

    enum class CustomShaderPresenceFlag {
        Vertex = 1 << 0,
        Fragment = 1 << 1
    };
    Q_DECLARE_FLAGS(CustomShaderPresence, CustomShaderPresenceFlag)

    enum class RenderFlag {
        Blending = 1 << 0,
        ScreenTexture = 1 << 1,
        DepthTexture = 1 << 2,
        AoTexture = 1 << 3,
        OverridesPosition = 1 << 4,
        ProjectionMatrix = 1 << 5,
        InverseProjectionMatrix = 1 << 6,
        ScreenMipTexture = 1 << 7,
        VarColor = 1 << 8
    };
    Q_DECLARE_FLAGS(RenderFlags, RenderFlag)

    using Flag = QSSGRenderNode::Flag;
    Q_DECLARE_FLAGS(Flags, Flag)

    QByteArray m_shaderPathKey;
    CustomShaderPresence m_customShaderPresence;

    QVector<TextureProperty> m_textureProperties;
    QVector<Property> m_properties;

    QSSGRenderLightmaps m_lightmaps;
    QSSGRenderImage *m_iblProbe = nullptr;
    QSSGRenderImage *m_emissiveMap = nullptr;
    QSSGCullFaceMode m_cullMode = QSSGCullFaceMode::Back;
    RenderFlags m_renderFlags;
    QRhiGraphicsPipeline::BlendFactor m_srcBlend;
    QRhiGraphicsPipeline::BlendFactor m_dstBlend;
    float m_lineWidth = 1.0f;

    QSSGRenderGraphObject *m_nextSibling = nullptr;

    ShadingMode m_shadingMode = ShadingMode::Shaded;

    Flags m_flags;
    bool m_alwaysDirty = false;
    bool m_dirtyFlagWithInFrame = false;

    bool isDirty() const { return m_flags.testFlag(Flag::Dirty) || m_dirtyFlagWithInFrame || m_alwaysDirty; }

    void updateDirtyForFrame()
    {
        m_dirtyFlagWithInFrame = m_flags.testFlag(Flag::Dirty);
        m_flags.setFlag(Flag::Dirty, false);
    }

    QSSGShaderMaterialAdapter *adapter = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRenderCustomMaterial::CustomShaderPresence)

QT_END_NAMESPACE

#endif
