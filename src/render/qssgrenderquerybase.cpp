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
#include "qssgrenderquerybase_p.h"
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrenderprogrampipeline_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderQueryBase::QSSGRenderQueryBase(const QSSGRef<QSSGRenderContext> &context)
    : m_context(context), m_backend(context->backend())
{
    m_handle = m_backend->createQuery();
}

QSSGRenderQueryBase::~QSSGRenderQueryBase()
{
    if (m_handle)
        m_backend->releaseQuery(m_handle);
}
QT_END_NAMESPACE
