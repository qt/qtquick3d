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

#ifndef Q_QUICK3D_H
#define Q_QUICK3D_H

#include <QtGui/qsurfaceformat.h>
#include <QtQuick3D/qtquick3dglobal.h>

QT_BEGIN_NAMESPACE

class QQuick3D {
public:
static Q_QUICK3D_EXPORT QSurfaceFormat idealSurfaceFormat(int samples = -1);
};

QT_END_NAMESPACE

#endif
