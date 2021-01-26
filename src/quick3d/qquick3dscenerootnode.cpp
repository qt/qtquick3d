/****************************************************************************
**
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

#include "qquick3dscenerootnode_p.h"

QT_BEGIN_NAMESPACE

QQuick3DSceneRootNode::QQuick3DSceneRootNode(QQuick3DViewport *view3D, QQuick3DNode *parent)
    : QQuick3DNode(parent)
    , m_view3D(view3D)
{
}

QQuick3DSceneRootNode::~QQuick3DSceneRootNode()
{
}


QQuick3DViewport *QQuick3DSceneRootNode::view3D()
{
    return m_view3D;
}

QT_END_NAMESPACE
