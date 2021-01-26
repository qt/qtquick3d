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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrenderdatabuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderDataBuffer::QSSGRenderDataBuffer(const QSSGRef<QSSGRenderContext> &context,
                                               QSSGRenderBufferType bindFlags,
                                               QSSGRenderBufferUsageType usageType,
                                               QSSGByteView data)
    : m_context(context)
    , m_backend(context->backend())
    , m_usageType(usageType)
    , m_type(bindFlags)
    , m_bufferData(data)
    , m_bufferCapacity(data.size())
    , m_bufferSize(data.size())
    , m_mapped(false)
{
    m_handle = m_backend->createBuffer(bindFlags, usageType, data);
}

QSSGRenderDataBuffer::~QSSGRenderDataBuffer()
{
    if (m_handle)
        m_backend->releaseBuffer(m_handle);
}

QSSGByteRef QSSGRenderDataBuffer::mapBuffer()
{
    // don't map twice
    if (m_mapped) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to map a mapped buffer");
        Q_ASSERT(false);
    }

    quint8 *pData = (quint8 *)m_backend->mapBuffer(m_handle,
                                                   m_type,
                                                   0,
                                                   m_bufferSize,
                                                   QSSGRenderBufferAccessFlags(QSSGRenderBufferAccessTypeValues::Read
                                                                                 | QSSGRenderBufferAccessTypeValues::Write));

    m_bufferData = toDataView(pData, (quint32)m_bufferSize);
    m_bufferCapacity = (quint32)m_bufferSize;

    // currently we return a reference to the system memory
    m_mapped = true;
    return QSSGByteRef(pData, qint32(m_bufferSize));
}

QSSGByteRef QSSGRenderDataBuffer::mapBufferRange(size_t offset, size_t size, QSSGRenderBufferAccessFlags flags)
{
    // don't map twice
    if (m_mapped) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to map a mapped buffer");
        Q_ASSERT(false);
    }
    // don't map out of range
    if ((m_bufferSize < (offset + size)) || (size == 0)) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to map out of buffer range");
        Q_ASSERT(false);
    }

    quint8 *pData = (quint8 *)m_backend->mapBuffer(m_handle, m_type, offset, size, flags);

    m_bufferData = toDataView(pData, (quint32)size);
    m_bufferCapacity = (quint32)size;

    // currently we return a reference to the system memory
    m_mapped = true;
    return QSSGByteRef(pData, qint32(m_bufferSize));
}

void QSSGRenderDataBuffer::unmapBuffer()
{
    if (m_mapped) {
        // update hardware
        m_backend->unmapBuffer(m_handle, m_type);
        m_mapped = false;
        m_bufferData.clear();
    }
}

void QSSGRenderDataBuffer::updateBuffer(QSSGByteView data)
{
    // don't update a mapped buffer
    if (m_mapped) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to update a mapped buffer");
        Q_ASSERT(false);
    }

    m_bufferData = data;
    m_bufferCapacity = data.mSize;
    // update hardware
    m_backend->updateBuffer(m_handle, m_type, m_usageType, data);
}
QT_END_NAMESPACE
