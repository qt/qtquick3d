// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
