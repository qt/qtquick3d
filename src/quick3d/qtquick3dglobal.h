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

#ifndef QTQUICK3DGLOBAL_H
#define QTQUICK3DGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#if defined(QT_BUILD_QUICK3D_LIB)
#define Q_QUICK3D_EXPORT Q_DECL_EXPORT
#else
#define Q_QUICK3D_EXPORT Q_DECL_IMPORT
#endif
#else
#define Q_QUICK3D_EXPORT
#endif

QT_END_NAMESPACE

void Q_QUICK3D_EXPORT qml_register_types_QtQuick3D();

#endif // QTQUICK3DGLOBAL_H
