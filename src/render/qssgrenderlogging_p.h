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


#ifndef QSSGRENDERLOGGING_H
#define QSSGRENDERLOGGING_H

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

#include <QtQuick3DRender/private/qtquick3drenderglobal_p.h>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(RENDER_GL_ERROR)
Q_DECLARE_LOGGING_CATEGORY(RENDER_INVALID_PARAMETER)
Q_DECLARE_LOGGING_CATEGORY(RENDER_INVALID_OPERATION)
Q_DECLARE_LOGGING_CATEGORY(RENDER_OUT_OF_MEMORY)
Q_DECLARE_LOGGING_CATEGORY(RENDER_INTERNAL_ERROR)
Q_DECLARE_LOGGING_CATEGORY(RENDER_PERF_WARNING)
Q_DECLARE_LOGGING_CATEGORY(RENDER_PERF_INFO)
Q_DECLARE_LOGGING_CATEGORY(RENDER_TRACE_INFO)
Q_DECLARE_LOGGING_CATEGORY(RENDER_WARNING)
Q_DECLARE_LOGGING_CATEGORY(RENDER_SHADER_INFO)

QT_END_NAMESPACE

#endif // QSSGRENDERLOGGING_H
