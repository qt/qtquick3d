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

#include "qssgrenderresourcebufferobjects_p.h"

QT_BEGIN_NAMESPACE

QSSGResourceFrameBuffer::QSSGResourceFrameBuffer(const QSSGRef<QSSGResourceManager> &mgr)
    : m_resourceManager(mgr)
{
}

QSSGResourceFrameBuffer::~QSSGResourceFrameBuffer()
{
    releaseFrameBuffer();
}

bool QSSGResourceFrameBuffer::ensureFrameBuffer()
{
    if (!m_frameBuffer) {
        m_frameBuffer = m_resourceManager->allocateFrameBuffer();
        return true;
    }
    return false;
}

void QSSGResourceFrameBuffer::releaseFrameBuffer()
{
    if (m_frameBuffer) {
        m_resourceManager->release(m_frameBuffer);
    }
}

QSSGResourceRenderBuffer::QSSGResourceRenderBuffer(const QSSGRef<QSSGResourceManager> &mgr)
    : m_resourceManager(mgr)
{
}

QSSGResourceRenderBuffer::~QSSGResourceRenderBuffer()
{
    releaseRenderBuffer();
}

bool QSSGResourceRenderBuffer::ensureRenderBuffer(qint32 width, qint32 height, QSSGRenderRenderBufferFormat storageFormat)
{
    if (m_renderBuffer == nullptr || m_dimensions.width() != width || m_dimensions.height() != height || m_storageFormat != storageFormat) {
        if (m_renderBuffer == nullptr || m_storageFormat != storageFormat) {
            releaseRenderBuffer();
            m_renderBuffer = m_resourceManager->allocateRenderBuffer(width, height, storageFormat);
        } else
            m_renderBuffer->setSize(QSize(width, height));
        m_dimensions = m_renderBuffer->size();
        m_storageFormat = m_renderBuffer->storageFormat();
        return true;
    }
    return false;
}

void QSSGResourceRenderBuffer::releaseRenderBuffer()
{
    if (m_renderBuffer) {
        m_resourceManager->release(m_renderBuffer);
        m_renderBuffer = nullptr;
    }
}

QT_END_NAMESPACE
