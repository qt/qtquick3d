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

#include <QtQuick3DRender/private/qssgrenderinputassembler_p.h>
#include <QtQuick3DRender/private/qssgrenderattriblayout_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

///< constructor
QSSGRenderInputAssembler::QSSGRenderInputAssembler(const QSSGRef<QSSGRenderContext> &context,
                                                       const QSSGRef<QSSGRenderAttribLayout> &attribLayout,
                                                       QSSGDataView<QSSGRef<QSSGRenderVertexBuffer>> buffers,
                                                       const QSSGRef<QSSGRenderIndexBuffer> &indexBuffer,
                                                       QSSGDataView<quint32> strides,
                                                       QSSGDataView<quint32> offsets,
                                                       QSSGRenderDrawMode primType,
                                                       quint32 patchVertexCount)
    : m_context(context)
    , m_backend(context->backend())
    , m_attribLayout(attribLayout)
    , m_indexBuffer(indexBuffer)
    , m_drawMode(primType)
    , m_patchVertexCount(patchVertexCount)
{
    // we cannot currently attach more than 16  vertex buffers
    Q_ASSERT(buffers.size() < 16);
    // if primitive is "Patch" we need a patch per vertex count > 0
    Q_ASSERT(m_drawMode != QSSGRenderDrawMode::Patches || m_patchVertexCount > 1);

    quint32 entrySize = sizeof(QSSGRenderBackend::QSSGRenderBackendBufferObject) * buffers.size();
    QSSGRenderBackend::QSSGRenderBackendBufferObject *bufferHandle = static_cast<QSSGRenderBackend::QSSGRenderBackendBufferObject *>(
            ::malloc(entrySize));
    // setup vertex buffer backend handle array
    for (int idx = 0; idx != buffers.size(); ++idx) {
        m_vertexBuffers.push_back(buffers.mData[idx]);
        bufferHandle[idx] = buffers.mData[idx]->handle();
    };

    m_vertexbufferHandles = toDataView(bufferHandle, buffers.size());

    m_handle = m_backend->createInputAssembler(m_attribLayout->handle(),
                                                              m_vertexbufferHandles,
                                                              (m_indexBuffer) ? m_indexBuffer->handle() : nullptr,
                                                              strides,
                                                              offsets,
                                                              patchVertexCount);

    // attribLayout->addRef();
}

///< destructor
QSSGRenderInputAssembler::~QSSGRenderInputAssembler()
{
    // m_attribLayout->release();

    if (m_handle) {
        m_backend->releaseInputAssembler(m_handle);
    }
    // ### sketchy
    ::free(const_cast<QSSGRenderBackend::QSSGRenderBackendBufferObject *>(m_vertexbufferHandles.mData));
}

const QSSGRef<QSSGRenderIndexBuffer> &QSSGRenderInputAssembler::indexBuffer()
{
    return m_indexBuffer;
}

quint32 QSSGRenderInputAssembler::indexCount() const
{
    return (m_indexBuffer) ? m_indexBuffer->numIndices() : 0;
}

quint32 QSSGRenderInputAssembler::vertexCount() const
{
    // makes only sense if we have a single vertex buffer
    Q_ASSERT(m_vertexBuffers.size() == 1);

    return m_vertexBuffers[0]->numVertexes();
}

QT_END_NAMESPACE
