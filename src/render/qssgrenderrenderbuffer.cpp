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

#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderRenderBuffer::QSSGRenderRenderBuffer(const QSSGRef<QSSGRenderContext> &context,
                                                   QSSGRenderRenderBufferFormat format,
                                                   quint32 width,
                                                   quint32 height)
    : m_context(context), m_backend(context->backend()), m_width(width), m_height(height), m_storageFormat(format), m_handle(nullptr)
{
    setSize(QSize(width, height));
}

QSSGRenderRenderBuffer::~QSSGRenderRenderBuffer()
{
    m_backend->releaseRenderbuffer(m_handle);
    m_handle = nullptr;
}

void QSSGRenderRenderBuffer::setSize(const QSize &inDimensions)
{
    qint32 maxWidth, maxHeight;
    m_width = inDimensions.width();
    m_height = inDimensions.height();

    // get max size and clamp to max value
    m_context->maxTextureSize(maxWidth, maxHeight);
    if (m_width > maxWidth || m_height > maxHeight) {
        qCCritical(RENDER_INVALID_OPERATION, "Width or height is greater than max texture size (%d, %d)", maxWidth, maxHeight);
        m_width = qMin(m_width, maxWidth);
        m_height = qMin(m_height, maxHeight);
    }

    bool success = true;

    if (m_handle == nullptr)
        m_handle = m_backend->createRenderbuffer(m_storageFormat, m_width, m_height);
    else
        success = m_backend->resizeRenderbuffer(m_handle, m_storageFormat, m_width, m_height);

    if (m_handle == nullptr || !success) {
        // We could try smaller sizes
        Q_ASSERT(false);
        qCCritical(RENDER_INTERNAL_ERROR,
                   "Unable to create render buffer %s, %dx%d",
                   toString(m_storageFormat),
                   m_width,
                   m_height);
    }
}

QT_END_NAMESPACE
