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
#include <QtQuick3DRender/private/qssgrenderprogrampipeline_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderProgramPipeline::QSSGRenderProgramPipeline(const QSSGRef<QSSGRenderContext> &context)
    : m_context(context)
    , m_backend(context->backend())
    , m_program(nullptr)
    , m_vertexProgram(nullptr)
    , m_fragmentProgram(nullptr)
    , m_tessControlProgram(nullptr)
    , m_tessEvalProgram(nullptr)
    , m_geometryProgram(nullptr)
    , m_computProgram(nullptr)
{
    m_handle = m_backend->createProgramPipeline();
}

QSSGRenderProgramPipeline::~QSSGRenderProgramPipeline()
{
    if (m_handle) {
        m_backend->releaseProgramPipeline(m_handle);
    }

    if (m_vertexProgram)
        m_vertexProgram.clear();
    if (m_fragmentProgram)
        m_fragmentProgram.clear();
    if (m_tessControlProgram)
        m_tessControlProgram.clear();
    if (m_tessEvalProgram)
        m_tessEvalProgram.clear();
    if (m_geometryProgram)
        m_geometryProgram.clear();
}

bool QSSGRenderProgramPipeline::isValid()
{
    return (m_handle != nullptr);
}

void QSSGRenderProgramPipeline::setProgramStages(const QSSGRef<QSSGRenderShaderProgram> &inProgram, QSSGRenderShaderTypeFlags flags)
{
    bool bDirty = false;

    if (flags & QSSGRenderShaderTypeValue::Vertex && inProgram != m_vertexProgram) {
        m_vertexProgram = inProgram;
        bDirty = true;
    }
    if (flags & QSSGRenderShaderTypeValue::Fragment && inProgram != m_fragmentProgram) {
        m_fragmentProgram = inProgram;
        bDirty = true;
    }
    if (flags & QSSGRenderShaderTypeValue::TessControl && inProgram != m_tessControlProgram) {
        m_tessControlProgram = inProgram;
        bDirty = true;
    }
    if (flags & QSSGRenderShaderTypeValue::TessEvaluation && inProgram != m_tessEvalProgram) {
        m_tessEvalProgram = inProgram;
        bDirty = true;
    }
    if (flags & QSSGRenderShaderTypeValue::Geometry && inProgram != m_geometryProgram) {
        m_geometryProgram = inProgram;
        bDirty = true;
    }

    if (bDirty) {
        m_backend->setProgramStages(m_handle, flags, (inProgram) ? inProgram->handle() : nullptr);
    }
}

void QSSGRenderProgramPipeline::bind()
{
    m_backend->setActiveProgramPipeline(m_handle);
}

QSSGRef<QSSGRenderShaderProgram> QSSGRenderProgramPipeline::vertexStage()
{
    return m_vertexProgram;
}

QT_END_NAMESPACE
