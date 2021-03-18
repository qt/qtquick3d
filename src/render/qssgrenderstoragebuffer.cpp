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

#include <QtQuick3DRender/private/qssgrenderstoragebuffer_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderStorageBuffer::QSSGRenderStorageBuffer(const QSSGRef<QSSGRenderContext> &context,
                                                     const QByteArray &bufferName,
                                                     QSSGRenderBufferUsageType usageType,
                                                     QSSGByteView data,
                                                     QSSGRenderDataBuffer *pBuffer)
    : QSSGRenderDataBuffer(context, QSSGRenderBufferType::Storage, usageType, data)
    , m_name(bufferName)
    , m_wrappedBuffer(pBuffer)
    , m_dirty(true)
{
    Q_ASSERT(context->supportsStorageBuffer());
    context->registerStorageBuffer(this);
}

QSSGRenderStorageBuffer::~QSSGRenderStorageBuffer()
{

    m_context->bufferDestroyed(this);
}

void QSSGRenderStorageBuffer::bind()
{
    if (m_mapped) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to Bind a locked buffer");
        Q_ASSERT(false);
    }

    if (m_wrappedBuffer)
        m_wrappedBuffer->bind();
    else
        m_backend->bindBuffer(m_handle, m_type);
}

void QSSGRenderStorageBuffer::bindToShaderProgram(quint32 index)
{
    m_backend->programSetStorageBuffer(index, (m_wrappedBuffer) ? m_wrappedBuffer->handle() : m_handle);
}

void QSSGRenderStorageBuffer::update()
{
    // we only update the buffer if it is dirty and we actually have some data
    if (m_dirty && m_bufferData.size()) {
        m_backend->updateBuffer(m_handle, m_type, m_usageType, m_bufferData);
        m_dirty = false;
    }
}

void QSSGRenderStorageBuffer::updateData(qint32 offset, QSSGByteView data)
{
    // we only update the buffer if it is not just a wrapper
    if (!m_wrappedBuffer)
        m_backend->updateBufferRange(m_handle, m_type, offset, data);
}

QT_END_NAMESPACE
