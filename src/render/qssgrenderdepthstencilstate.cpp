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
#include "qssgrenderdepthstencilstate_p.h"

QT_BEGIN_NAMESPACE

QSSGRenderDepthStencilState::QSSGRenderDepthStencilState(const QSSGRef<QSSGRenderContext> &context,
                                                             bool enableDepth,
                                                             bool depthMask,
                                                             QSSGRenderBoolOp depthFunc,
                                                             bool enableStencil,
                                                             QSSGRenderStencilFunction &stencilFuncFront,
                                                             QSSGRenderStencilFunction &stencilFuncBack,
                                                             QSSGRenderStencilOperation &depthStencilOpFront,
                                                             QSSGRenderStencilOperation &depthStencilOpBack)
    : m_backend(context->backend())
    , m_depthEnabled(enableDepth)
    , m_depthMask(depthMask)
    , m_stencilEnabled(enableStencil)
    , m_depthFunc(depthFunc)
    , m_stencilFuncFront(stencilFuncFront)
    , m_stencilFuncBack(stencilFuncBack)
    , m_depthStencilOpFront(depthStencilOpFront)
    , m_depthStencilOpBack(depthStencilOpBack)
{
    // create backend handle
    m_handle = m_backend->createDepthStencilState(enableDepth, depthMask, depthFunc, enableStencil, stencilFuncFront, stencilFuncBack, depthStencilOpFront, depthStencilOpBack);
}

QSSGRenderDepthStencilState::~QSSGRenderDepthStencilState()
{
    if (m_handle) {
        m_backend->releaseDepthStencilState(m_handle);
    }
}

QT_END_NAMESPACE
