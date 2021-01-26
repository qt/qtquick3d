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

#include "qssgrenderattriblayout_p.h"
#include "qssgrendercontext_p.h"

QT_BEGIN_NAMESPACE

///< constructor
QSSGRenderAttribLayout::QSSGRenderAttribLayout(const QSSGRef<QSSGRenderContext> &context,
                                                   QSSGDataView<QSSGRenderVertexBufferEntry> attribs)
    : m_context(context), m_backend(context->backend())
{
    m_attribLayoutHandle = m_backend->createAttribLayout(attribs);
    Q_ASSERT(m_attribLayoutHandle);
}

///< destructor
QSSGRenderAttribLayout::~QSSGRenderAttribLayout()
{
    if (m_attribLayoutHandle) {
        m_backend->releaseAttribLayout(m_attribLayoutHandle);
    }
}
QT_END_NAMESPACE
