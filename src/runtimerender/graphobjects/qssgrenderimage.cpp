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
#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include <QtQuick/QSGTexture>

QT_BEGIN_NAMESPACE

QSSGRenderImage::QSSGRenderImage()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::Image)
{
    m_flags.setFlag(Flag::Active);
    m_flags.setFlag(Flag::Dirty);
    m_flags.setFlag(Flag::TransformDirty);
}

QSSGRenderImage::~QSSGRenderImage() = default;


bool QSSGRenderImage::clearDirty(const QSSGRef<QSSGBufferManager> &inBufferManager, bool forIbl)
{
    bool wasDirty = m_flags.testFlag(Flag::Dirty);
    m_flags.setFlag(Flag::Dirty, false);

    if (wasDirty) {
        QSSGRenderImageTextureData newImage;
        if (m_qsgTexture)
            newImage = inBufferManager->loadRenderImage(m_qsgTexture);
        else
            newImage = inBufferManager->loadRenderImage(m_imagePath, m_format, false, forIbl);

        if (newImage.m_texture != m_textureData.m_texture)
            m_textureData = newImage;
    }

    if (m_flags.testFlag(Flag::TransformDirty)) {
        calculateTextureTransform();
        return true;
    }
    return wasDirty;
}

void QSSGRenderImage::calculateTextureTransform()
{
    m_flags.setFlag(Flag::TransformDirty, false);

    if (m_flipV) {
        m_textureTransform = QMatrix4x4(1.f,  0.f, 0.f, 0.f,
                                        0.f, -1.f, 0.f, 1.f,
                                        0.f,  0.f, 1.f, 0.f,
                                        0.f,  0.f, 0.f, 1.f);
    } else {
        m_textureTransform = QMatrix4x4();
    }

    QMatrix4x4 pivot;
    QMatrix4x4 pivot_r;
    QMatrix4x4 translation;
    QMatrix4x4 rotation;
    QMatrix4x4 scale;

    pivot.translate(m_pivot.x(), m_pivot.y());
    pivot_r.translate(-m_pivot.x(), -m_pivot.y());
    translation.translate(m_position.x(), m_position.y());
    scale.scale(m_scale.x(), m_scale.y());
    rotation.rotate(m_rotation, QVector3D(0, 0, 1));

    m_textureTransform *= translation;
    m_textureTransform *= pivot;
    m_textureTransform *= rotation;
    m_textureTransform *= scale;
    m_textureTransform *= pivot_r;
}

bool QSSGRenderImage::isImageTransformIdentity() const
{
    if (m_mappingMode != MappingModes::Normal)
        return false;
    return m_textureTransform.isIdentity();
}

QSSGRenderImageTextureData::QSSGRenderImageTextureData() = default;
QSSGRenderImageTextureData::~QSSGRenderImageTextureData() = default;

QT_END_NAMESPACE
