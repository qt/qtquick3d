/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qssgruntimerenderlogging_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(GL_ERROR, "QSSG.gl_error")
Q_LOGGING_CATEGORY(INVALID_PARAMETER, "QSSG.invalid_parameter")
Q_LOGGING_CATEGORY(INVALID_OPERATION, "QSSG.invalid_operation")
Q_LOGGING_CATEGORY(OUT_OF_MEMORY, "QSSG.out_of_memory")
Q_LOGGING_CATEGORY(INTERNAL_ERROR, "QSSG.internal_error")
Q_LOGGING_CATEGORY(PERF_WARNING, "QSSG.perf_warning")
Q_LOGGING_CATEGORY(PERF_INFO, "QSSG.perf_info")
Q_LOGGING_CATEGORY(TRACE_INFO, "QSSG.trace_info")
Q_LOGGING_CATEGORY(WARNING, "QSSG.warning")

QT_END_NAMESPACE
