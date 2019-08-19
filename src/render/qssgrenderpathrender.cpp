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

#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrenderpathrender_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrenderpathspecification_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderPathRender::QSSGRenderPathRender(const QSSGRef<QSSGRenderContext> &context, size_t range)
    : m_context(context), m_backend(context->backend()), m_strokeWidth(0.0f)
{
    m_range = range;
    m_pathRenderHandle = m_backend->createPathNVObject(range);
}

QSSGRenderPathRender::~QSSGRenderPathRender()
{
    if (m_pathRenderHandle) {
        m_backend->releasePathNVObject(m_pathRenderHandle, m_range);
    }
}

void QSSGRenderPathRender::setPathSpecification(const QSSGRef<QSSGRenderPathSpecification> &inCommandBuffer)
{
    m_backend->setPathSpecification(m_pathRenderHandle,
                                    toDataView(inCommandBuffer->getPathCommands()),
                                    toDataView(inCommandBuffer->getPathCoords()));
}

QSSGBounds3 QSSGRenderPathRender::getPathObjectBoundingBox()
{
    return m_backend->getPathObjectBoundingBox(m_pathRenderHandle);
}

QSSGBounds3 QSSGRenderPathRender::getPathObjectFillBox()
{
    return m_backend->getPathObjectFillBox(m_pathRenderHandle);
}

QSSGBounds3 QSSGRenderPathRender::getPathObjectStrokeBox()
{
    return m_backend->getPathObjectStrokeBox(m_pathRenderHandle);
}

void QSSGRenderPathRender::setStrokeWidth(float inStrokeWidth)
{
    if (inStrokeWidth != m_strokeWidth) {
        m_strokeWidth = inStrokeWidth;
        m_backend->setStrokeWidth(m_pathRenderHandle, inStrokeWidth);
    }
}

float QSSGRenderPathRender::getStrokeWidth() const
{
    return m_strokeWidth;
}

void QSSGRenderPathRender::stencilStroke()
{
    m_backend->stencilStrokePath(m_pathRenderHandle);
}

void QSSGRenderPathRender::stencilFill()
{
    m_backend->stencilFillPath(m_pathRenderHandle);
}

QSSGRef<QSSGRenderPathRender> QSSGRenderPathRender::create(const QSSGRef<QSSGRenderContext> &context, size_t range)
{
    if (!context->supportsPathRendering())
        return nullptr;

    return QSSGRef<QSSGRenderPathRender>(new QSSGRenderPathRender(context, range));
}

QT_END_NAMESPACE
