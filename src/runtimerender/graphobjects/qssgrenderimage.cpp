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

#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgoffscreenrendermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include "qssgoffscreenrenderkey_p.h"
#include <QtQuick/QSGTexture>

QT_BEGIN_NAMESPACE

QSSGRenderImage::QSSGRenderImage()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::Image)
    , m_lastFrameOffscreenRenderer(nullptr)
    , m_parent(nullptr)
    , m_scale(1, 1)
    , m_pivot(0, 0)
    , m_rotation(0)
    , m_position(0, 0)
    , m_mappingMode(MappingModes::Normal)
    , m_horizontalTilingMode(QSSGRenderTextureCoordOp::ClampToEdge)
    , m_verticalTilingMode(QSSGRenderTextureCoordOp::ClampToEdge)
    , m_format(QSSGRenderTextureFormat::Unknown)
{
    m_flags.setFlag(Flag::Active);
    m_flags.setFlag(Flag::Dirty);
    m_flags.setFlag(Flag::TransformDirty);
}

QSSGRenderImage::~QSSGRenderImage() = default;

static void HandleOffscreenResult(QSSGRenderImage &theImage,
                                  QSSGRenderImageTextureData &newImage,
                                  QSSGOffscreenRenderResult &theResult,
                                  bool &replaceTexture,
                                  bool &wasDirty)
{
    newImage.m_texture = theResult.texture;
    newImage.m_textureFlags.setHasTransparency(theResult.hasTransparency);
    newImage.m_textureFlags.setPreMultiplied(true);
    wasDirty = wasDirty || theResult.hasChangedSinceLastFrame;
    theImage.m_lastFrameOffscreenRenderer = theResult.renderer;
    replaceTexture = true;
}

bool QSSGRenderImage::clearDirty(const QSSGRef<QSSGBufferManager> &inBufferManager,
                                   QSSGOffscreenRenderManager &inRenderManager,
                                   bool forIbl)
{
    bool wasDirty = m_flags.testFlag(Flag::Dirty);
    m_flags.setFlag(Flag::Dirty, false);
    QSSGRenderImageTextureData newImage;
    bool replaceTexture(false);
    if (!m_offscreenRendererId.isEmpty()) {
        QSSGOffscreenRenderResult theResult = inRenderManager.getRenderedItem(m_offscreenRendererId);
        HandleOffscreenResult(*this, newImage, theResult, replaceTexture, wasDirty);
    }

    if (wasDirty && newImage.m_texture == nullptr && m_qsgTexture) {
        newImage = inBufferManager->loadRenderImage(m_qsgTexture);
        replaceTexture = newImage.m_texture != m_textureData.m_texture;
    }

    if (wasDirty && newImage.m_texture == nullptr) {
        m_lastFrameOffscreenRenderer = nullptr;
        newImage = inBufferManager->loadRenderImage(m_imagePath, m_format, false, forIbl);
        replaceTexture = newImage.m_texture != m_textureData.m_texture;
    }

    if (replaceTexture) {
        wasDirty = true;
        m_textureData = newImage;
    }

    if (m_flags.testFlag(Flag::TransformDirty)) {
        wasDirty = true;
        calculateTextureTransform();
    }
    return wasDirty;
}

void QSSGRenderImage::calculateTextureTransform()
{
    m_flags.setFlag(Flag::TransformDirty, false);

    m_textureTransform = QMatrix4x4();

    QMatrix4x4 translation;
    QMatrix4x4 rotation;
    QMatrix4x4 scale;

    translation.translate(m_position.x(), m_position.y());
    scale.scale(m_scale.x(), m_scale.y());
    rotation.rotate(m_rotation, QVector3D(0, 0, 1));

    // Setup the pivot.
    m_textureTransform.translate(m_pivot.x(), m_pivot.y());
    m_textureTransform = m_textureTransform * rotation;
    m_textureTransform = m_textureTransform * scale;
    m_textureTransform = m_textureTransform * translation;
}

QSSGRenderImageTextureData::QSSGRenderImageTextureData() = default;
QSSGRenderImageTextureData::~QSSGRenderImageTextureData() = default;

QT_END_NAMESPACE
