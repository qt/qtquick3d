/****************************************************************************
**
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

#ifndef QSSGCUSTOMMATERIAL_H
#define QSSGCUSTOMMATERIAL_H

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

#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtCore/qurl.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3D/private/qquick3dshaderutils_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DCustomMaterial : public QQuick3DMaterial
{
    Q_OBJECT
    Q_PROPERTY(ShadingMode shadingMode READ shadingMode WRITE setShadingMode NOTIFY shadingModeChanged)
    Q_PROPERTY(QUrl fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged)
    Q_PROPERTY(QUrl vertexShader READ vertexShader WRITE setVertexShader NOTIFY vertexShaderChanged)
    Q_PROPERTY(bool hasTransparency READ hasTransparency WRITE setHasTransparency NOTIFY hasTransparencyChanged)
    Q_PROPERTY(BlendMode sourceBlend READ srcBlend WRITE setSrcBlend NOTIFY srcBlendChanged)
    Q_PROPERTY(BlendMode destinationBlend READ dstBlend WRITE setDstBlend NOTIFY dstBlendChanged)
    Q_PROPERTY(bool alwaysDirty READ alwaysDirty WRITE setAlwaysDirty NOTIFY alwaysDirtyChanged)
    Q_PROPERTY(ShaderKeyFlags shaderKey READ shaderKey WRITE setShaderKey NOTIFY shaderKeyChanged)

public:
    enum class ShadingMode
    {
        Unshaded,
        Shaded
    };
    Q_ENUM(ShadingMode)

    enum class BlendMode
    {
        NoBlend,
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate
    };
    Q_ENUM(BlendMode)

    enum class ShaderKeyValues
    {
        Diffuse = 1 << 0,
        Specular = 1 << 1,
        Cutout = 1 << 2,
        Refraction = 1 << 3,
        Transparent = 1 << 4,
        Transmissive = 1 << 5,
        Glossy = Diffuse | Specular
    };
    Q_ENUM(ShaderKeyValues)
    Q_DECLARE_FLAGS(ShaderKeyFlags, ShaderKeyValues)

    explicit QQuick3DCustomMaterial(QQuick3DObject *parent = nullptr);
    ~QQuick3DCustomMaterial() override;

    ShadingMode shadingMode() const;
    void setShadingMode(ShadingMode mode);

    QUrl vertexShader() const;
    void setVertexShader(const QUrl &url);

    QUrl fragmentShader() const;
    void setFragmentShader(const QUrl &url);

    bool hasTransparency() const;
    void setHasTransparency(bool hasTransparency);

    BlendMode srcBlend() const;
    void setSrcBlend(BlendMode mode);

    BlendMode dstBlend() const;
    void setDstBlend(BlendMode mode);

    bool alwaysDirty() const;
    void setAlwaysDirty(bool alwaysDirty);

    ShaderKeyFlags shaderKey() const;
    void setShaderKey(ShaderKeyFlags key);

Q_SIGNALS:
    void shadingModeChanged();
    void vertexShaderChanged();
    void fragmentShaderChanged();
    void hasTransparencyChanged();
    void srcBlendChanged();
    void dstBlendChanged();
    void alwaysDirtyChanged();
    void shaderKeyChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;

private Q_SLOTS:
    void onPropertyDirty();
    void onTextureDirty(QQuick3DShaderUtilsTextureInput *texture);

private:
    enum Dirty {
        TextureDirty = 0x1,
        PropertyDirty = 0x2,
        ShaderSettingsDirty = 0x4
    };

    void markDirty(QQuick3DCustomMaterial::Dirty type)
    {
        if (!(m_dirtyAttributes & quint32(type))) {
            m_dirtyAttributes |= quint32(type);
            update();
        }
    }

    quint32 m_dirtyAttributes = 0xffffffff;
    bool m_hasTransparency = false;
    BlendMode m_srcBlend = BlendMode::NoBlend;
    BlendMode m_dstBlend = BlendMode::NoBlend;
    ShadingMode m_shadingMode = ShadingMode::Shaded;
    QUrl m_vertexShader;
    QUrl m_fragmentShader;
    ShaderKeyFlags m_shaderKey;
    bool m_alwaysDirty = false;
};

QT_END_NAMESPACE

#endif // QSSGCUSTOMMATERIAL_H
