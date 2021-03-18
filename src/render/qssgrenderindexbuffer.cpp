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

#include <QtQuick3DRender/private/qssgrenderindexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderIndexBuffer::QSSGRenderIndexBuffer(const QSSGRef<QSSGRenderContext> &context,
                                                 QSSGRenderBufferUsageType usageType,
                                                 QSSGRenderComponentType componentType,
                                                 QSSGByteView data)
    : QSSGRenderDataBuffer(context, QSSGRenderBufferType::Index, usageType, data),
      m_componentType(componentType)
{
}

QSSGRenderIndexBuffer::~QSSGRenderIndexBuffer()
{
}

quint32 QSSGRenderIndexBuffer::numIndices() const
{
    quint32 dtypeSize = getSizeOfType(m_componentType);
    return m_bufferCapacity / dtypeSize;
}

void QSSGRenderIndexBuffer::draw(QSSGRenderDrawMode drawMode, quint32 count, quint32 offset)
{
    m_backend->drawIndexed(drawMode, count, m_componentType, reinterpret_cast<const void *>(quintptr(offset * getSizeOfType(m_componentType))));
}

void QSSGRenderIndexBuffer::bind()
{
    if (m_mapped) {
        qCCritical(RENDER_INVALID_OPERATION, "Attempting to Bind a locked buffer");
        Q_ASSERT(false);
    }

    m_backend->bindBuffer(m_handle, m_type);
}

QT_END_NAMESPACE
