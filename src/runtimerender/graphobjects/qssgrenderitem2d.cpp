/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/


#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick/QSGTexture>

QT_BEGIN_NAMESPACE

QSSGRenderItem2D::QSSGRenderItem2D()
    : QSSGRenderNode(QSSGRenderGraphObject::Type::Item2D)
{
}

QSSGRenderItem2D::~QSSGRenderItem2D()
{
    // Normally it will be deleted when m_renderer destroyed
    // by QQuick3DItem2D's connection
    // But if this backend node may suddenly be deleted,
    // it is safe to remain this deletion here.
    delete m_rp;
}

QT_END_NAMESPACE
