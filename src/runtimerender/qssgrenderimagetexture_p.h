// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_IMAGE_TEXTURE_H
#define QSSG_RENDER_IMAGE_TEXTURE_H

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

#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QRhiTexture;

enum class QSSGRenderImageTextureFlagValue
{
    HasTransparency = 1 << 0,
    RGBE8 = 1 << 1,
    Linear = 1 << 2
};

struct QSSGRenderImageTextureFlags : public QFlags<QSSGRenderImageTextureFlagValue>
{
    bool hasTransparency() const { return this->operator&(QSSGRenderImageTextureFlagValue::HasTransparency); }
    void setHasTransparency(bool inValue) { setFlag(QSSGRenderImageTextureFlagValue::HasTransparency, inValue); }

    bool isRgbe8() const { return this->operator&(QSSGRenderImageTextureFlagValue::RGBE8); }
    void setRgbe8(bool inValue) { setFlag(QSSGRenderImageTextureFlagValue::RGBE8, inValue); }

    bool isLinear() const { return this->operator&(QSSGRenderImageTextureFlagValue::Linear); }
    void setLinear(bool inValue) { setFlag(QSSGRenderImageTextureFlagValue::Linear, inValue); }
};

struct QSSGRenderImageTexture
{
    QRhiTexture *m_texture = nullptr; // not owned
    int m_mipmapCount = 0;
    QSSGRenderImageTextureFlags m_flags;
};

QT_END_NAMESPACE

#endif
