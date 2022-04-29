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

#ifndef QSSGITEMCHANGELISTENER_P_H
#define QSSGITEMCHANGELISTENER_P_H

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

#include <QtQuick3D/qquick3dobject.h>

QT_BEGIN_NAMESPACE

class QQuick3DObjectChangeListener
{
public:
    virtual ~QQuick3DObjectChangeListener() {}

    virtual void itemSiblingOrderChanged(QQuick3DObject *) {}
    virtual void itemVisibilityChanged(QQuick3DObject *) {}
    virtual void itemEnabledChanged(QQuick3DObject *) {}
    virtual void itemOpacityChanged(QQuick3DObject *) {}
    virtual void itemDestroyed(QQuick3DObject *) {}
    virtual void itemChildAdded(QQuick3DObject *, QQuick3DObject * /* child */) {}
    virtual void itemChildRemoved(QQuick3DObject *, QQuick3DObject * /* child */) {}
    virtual void itemParentChanged(QQuick3DObject *, QQuick3DObject * /* parent */) {}
};

QT_END_NAMESPACE

#endif // QSSGITEMCHANGELISTENER_P_H
