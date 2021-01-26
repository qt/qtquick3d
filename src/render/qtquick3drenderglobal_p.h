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

#ifndef QTQUICK3DRENDERGLOBAL_P_H
#define QTQUICK3DRENDERGLOBAL_P_H

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

#include <QtCore/qglobal.h>

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

#define Q_QUICK3DRENDER_PRIVATE_EXPORT Q_QUICK3DRENDER_EXPORT

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#if defined(QT_BUILD_QUICK3DRENDER_LIB)
#define Q_QUICK3DRENDER_EXPORT Q_DECL_EXPORT
#else
#define Q_QUICK3DRENDER_EXPORT Q_DECL_IMPORT
#endif
#else
#define Q_QUICK3DRENDER_EXPORT
#endif

QT_END_NAMESPACE

#endif // QTQUICK3DRENDERGLOBAL_P_H
