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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendersync_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderSync::QSSGRenderSync(const QSSGRef<QSSGRenderContext> &context)
    : m_backend(context->backend()), m_handle(nullptr)
{
}

QSSGRenderSync::~QSSGRenderSync()
{
    if (m_handle)
        m_backend->releaseSync(m_handle);
}

void QSSGRenderSync::sync()
{
    // On every sync call we need to create a new sync object
    // A sync object can only be used once

    // First delete the old object
    // We can safely do this because it is actually not deleted until
    // it is unused
    if (m_handle)
        m_backend->releaseSync(m_handle);

    m_handle = m_backend->createSync(QSSGRenderSyncType::GpuCommandsComplete, QSSGRenderSyncFlags());
}

void QSSGRenderSync::wait()
{
    // wait until the sync object is signaled or a timeout happens
    if (m_handle)
        m_backend->waitSync(m_handle, QSSGRenderCommandFlushFlags(), 0);
}

QSSGRef<QSSGRenderSync> QSSGRenderSync::create(const QSSGRef<QSSGRenderContext> &context)
{
    if (!context->supportsCommandSync())
        return nullptr;

    return QSSGRef<QSSGRenderSync>(new QSSGRenderSync(context));
}

QT_END_NAMESPACE
