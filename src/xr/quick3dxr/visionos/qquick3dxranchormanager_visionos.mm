// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxranchormanager_visionos_p.h"

#include "../qopenxrspatialanchor_p.h"

QT_BEGIN_NAMESPACE


QQuick3DXrAnchorManager *QQuick3DXrAnchorManager::instance()
{
    static QQuick3DXrAnchorManager instance;
    return &instance;
}

QQuick3DXrAnchorManager::QQuick3DXrAnchorManager(QObject *parent)
    : QObject(parent)
{
}


QQuick3DXrAnchorManager::~QQuick3DXrAnchorManager()
{

}

void QQuick3DXrAnchorManager::requestSceneCapture()
{
    Q_UNIMPLEMENTED(); qWarning() << "QQuick3DXrAnchorManager::requestSceneCapture() not implemented on this platform";
}

bool QQuick3DXrAnchorManager::queryAllAnchors()
{
    Q_UNIMPLEMENTED(); qWarning() << "QQuick3DXrAnchorManager::queryAllAnchors() not implemented on this platform";
    return false;
}

const QList<QQuick3DXrSpatialAnchor *> &QQuick3DXrAnchorManager::anchors() const
{
    static QList<QQuick3DXrSpatialAnchor *> anchors;
    Q_UNIMPLEMENTED(); qWarning() << "QQuick3DXrAnchorManager::anchors() not implemented on this platform";
    return anchors;
}

bool QQuick3DXrAnchorManager::setupSpatialAnchor(QtQuick3DXr::XrSpaceId space, QQuick3DXrSpatialAnchor &anchor)
{
    Q_UNUSED(space);
    Q_UNUSED(anchor);
    Q_UNIMPLEMENTED(); qWarning() << "QQuick3DXrAnchorManager::setupSpatialAnchor() not implemented on this platform";

    return false;
}

QT_END_NAMESPACE
