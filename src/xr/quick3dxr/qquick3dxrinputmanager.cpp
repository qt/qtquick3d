// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrinputmanager_p.h"

#if defined(Q_OS_VISIONOS)
# include "visionos/qquick3dxrinputmanager_visionos_p.h"
#else
# include "openxr/qopenxrinputmanager_p.h"
#endif

QT_BEGIN_NAMESPACE

QQuick3DXrInputManager *QQuick3DXrInputManager::instance()
{
    static QQuick3DXrInputManager instance;
    return &instance;
}

QQuick3DXrHandInput *QQuick3DXrInputManager::leftHandInput() const
{
    Q_D(const QQuick3DXrInputManager);
    return d->leftHandInput();
}

QQuick3DXrHandInput *QQuick3DXrInputManager::rightHandInput() const
{
    Q_D(const QQuick3DXrInputManager);
    return d->rightHandInput();
}

QQuick3DXrHandTrackerInput *QQuick3DXrInputManager::leftHandTrackerInput() const
{
    Q_D(const QQuick3DXrInputManager);
    return d->leftHandTrackerInput();
}

QQuick3DXrHandTrackerInput *QQuick3DXrInputManager::rightHandTrackerInput() const
{
    Q_D(const QQuick3DXrInputManager);
    return d->rightHandTrackerInput();
}

bool QQuick3DXrInputManager::isValid() const
{
    Q_D(const QQuick3DXrInputManager);
    return d->isValid();
}

QQuick3DXrInputManager::QQuick3DXrInputManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new QQuick3DXrInputManagerPrivate(*this))
{

}

QQuick3DXrInputManager::~QQuick3DXrInputManager()
{

}

QT_END_NAMESPACE
