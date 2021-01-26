/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSSG_RENDER_IMAGE_TEXTURE_DATA_H
#define QSSG_RENDER_IMAGE_TEXTURE_DATA_H

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

#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>

QT_BEGIN_NAMESPACE
// forward declararion
class QSSGRenderPrefilterTexture;

enum class QSSGImageTextureFlagValue
{
    HasTransparency = 1,
    InvertUVCoords = 1 << 1,
    PreMultiplied = 1 << 2,
};

struct QSSGRenderImageTextureFlags : public QFlags<QSSGImageTextureFlagValue>
{
    bool hasTransparency() const { return this->operator&(QSSGImageTextureFlagValue::HasTransparency); }
    void setHasTransparency(bool inValue) { setFlag(QSSGImageTextureFlagValue::HasTransparency, inValue); }

    bool isInvertUVCoords() const { return this->operator&(QSSGImageTextureFlagValue::InvertUVCoords); }
    void setInvertUVCoords(bool inValue) { setFlag(QSSGImageTextureFlagValue::InvertUVCoords, inValue); }

    bool isPreMultiplied() const { return this->operator&(QSSGImageTextureFlagValue::PreMultiplied); }
    void setPreMultiplied(bool inValue) { setFlag(QSSGImageTextureFlagValue::PreMultiplied, inValue); }
};

struct QSSGRenderImageTextureData
{
    QSSGRef<QSSGRenderTexture2D> m_texture;
    QSSGRenderImageTextureFlags m_textureFlags;
    QSSGRef<QSSGRenderPrefilterTexture> m_bsdfMipMap;

    QSSGRenderImageTextureData();
    ~QSSGRenderImageTextureData();

    bool operator!=(const QSSGRenderImageTextureData &inOther)
    {
        return m_texture != inOther.m_texture || m_textureFlags != inOther.m_textureFlags || m_bsdfMipMap != inOther.m_bsdfMipMap;
    }
};
QT_END_NAMESPACE

#endif
