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

#include <QtQuick3DRender/private/qssgrendertimerquery_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderTimerQuery::QSSGRenderTimerQuery(const QSSGRef<QSSGRenderContext> &context)
    : QSSGRenderQueryBase(context)
{
}

QSSGRenderTimerQuery::~QSSGRenderTimerQuery() = default;

void QSSGRenderTimerQuery::begin()
{
    m_backend->beginQuery(m_handle, QSSGRenderQueryType::Timer);
}

void QSSGRenderTimerQuery::end()
{
    m_backend->endQuery(m_handle, QSSGRenderQueryType::Timer);
}

void QSSGRenderTimerQuery::result(quint32 *params)
{
    m_backend->getQueryResult(m_handle, QSSGRenderQueryResultType::Result, params);
}

void QSSGRenderTimerQuery::result(quint64 *params)
{
    m_backend->getQueryResult(m_handle, QSSGRenderQueryResultType::Result, params);
}

void QSSGRenderTimerQuery::setTimerQuery()
{
    m_backend->setQueryTimer(m_handle);
}

QSSGRef<QSSGRenderTimerQuery> QSSGRenderTimerQuery::create(const QSSGRef<QSSGRenderContext> &context)
{
    if (!context->supportsTimerQuery())
        return nullptr;

    return QSSGRef<QSSGRenderTimerQuery>(new QSSGRenderTimerQuery(context));
}
QT_END_NAMESPACE
