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


#include "qssgrendermaterialshadergenerator_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

QT_BEGIN_NAMESPACE

QSSGMaterialShaderGeneratorInterface::QSSGMaterialShaderGeneratorInterface(QSSGRenderContextInterface *renderContext)
    : m_renderContext(renderContext),
      m_programGenerator(m_renderContext->shaderProgramGenerator())

{}

QSSGMaterialShaderGeneratorInterface::~QSSGMaterialShaderGeneratorInterface() {}

QT_END_NAMESPACE
