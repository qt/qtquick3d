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

#ifndef QSSG_RENDERABLE_IMAGE_H
#define QSSG_RENDERABLE_IMAGE_H

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
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>

QT_BEGIN_NAMESPACE

// Some precomputed information on a given image. When generating a renderable,
// the layer preparation step goes through all the possible images on a
// material (which includes all regular texture maps, but does not include
// light probes or custom texture properties for custom materials), and for
// each valid image it generates, if not already done, the QRhiTexture (for the
// current scene's window, and so render thread), and calculates some other
// data and flags.

struct QSSGRenderableImage
{
    enum class Type : quint8
    {
        Unknown = 0,
        Diffuse,
        Opacity,
        Specular,
        Emissive,
        Bump,
        SpecularAmountMap,
        Normal,
        Translucency,
        Roughness,
        BaseColor,
        Metalness,
        Occlusion,
        Height
    };
    const QSSGRenderImage &m_imageNode;
    QSSGRenderImageTexture m_texture;
    QSSGRenderableImage *m_nextImage;
    Type m_mapType;
    bool uvCoordsGenerated = false;
    QSSGRenderableImage(Type inMapType, const QSSGRenderImage &inImageNode, const QSSGRenderImageTexture &inTexture)
        : m_imageNode(inImageNode), m_texture(inTexture), m_nextImage(nullptr),  m_mapType(inMapType)
    {
    }
};
QT_END_NAMESPACE
#endif
