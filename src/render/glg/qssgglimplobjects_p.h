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

#ifndef QSSG_RENDER_GL_IMPL_OBJECTS_H
#define QSSG_RENDER_GL_IMPL_OBJECTS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRender/private/qssgopenglutil_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>

QT_BEGIN_NAMESPACE

// The set of all properties as they are currently set in hardware.
struct QSSGGLHardPropertyContext
{
    QSSGRef<QSSGRenderFrameBuffer> m_frameBuffer;
    QSSGRef<QSSGRenderShaderProgram> m_activeShader;
    QSSGRef<QSSGRenderProgramPipeline> m_activeProgramPipeline;
    QSSGRef<QSSGRenderInputAssembler> m_inputAssembler;
    QSSGRenderBlendFunctionArgument m_blendFunction;
    QSSGRenderBlendEquationArgument m_blendEquation;
    bool m_cullingEnabled = true;
    QSSGCullFaceMode m_cullFaceMode = QSSGCullFaceMode::Back;
    QSSGRenderBoolOp m_depthFunction = QSSGRenderBoolOp::Less;
    bool m_blendingEnabled = true;
    bool m_depthWriteEnabled = true;
    bool m_depthTestEnabled = true;
    bool m_stencilTestEnabled = false;
    bool m_scissorTestEnabled = true;
    bool m_colorWritesEnabled = true;
    bool m_multisampleEnabled = false;
    QRect m_scissorRect;
    QRect m_viewport;
    QVector4D m_clearColor{ 0.0, 0.0, 0.0, 1.0 };
};
QT_END_NAMESPACE
#endif
