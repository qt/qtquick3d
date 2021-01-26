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


#include "qssgrenderlogging_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(RENDER_GL_ERROR, "QSSG.gl_error")
Q_LOGGING_CATEGORY(RENDER_INVALID_PARAMETER, "QSSG.invalid_parameter")
Q_LOGGING_CATEGORY(RENDER_INVALID_OPERATION, "QSSG.invalid_operation")
Q_LOGGING_CATEGORY(RENDER_OUT_OF_MEMORY, "QSSG.out_of_memory")
Q_LOGGING_CATEGORY(RENDER_INTERNAL_ERROR, "QSSG.internal_error")
Q_LOGGING_CATEGORY(RENDER_PERF_WARNING, "QSSG.perf_warning")
Q_LOGGING_CATEGORY(RENDER_PERF_INFO, "QSSG.perf_info")
Q_LOGGING_CATEGORY(RENDER_TRACE_INFO, "QSSG.trace_info")
Q_LOGGING_CATEGORY(RENDER_WARNING, "QSSG.warning")
Q_LOGGING_CATEGORY(RENDER_SHADER_INFO, "QSSG.shader_info")

QT_END_NAMESPACE
