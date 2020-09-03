/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qssgrendercommands_p.h"

const char *QSSGCommand::typeAsString() const
{
    switch (m_type) {
    case CommandType::Unknown:
        return "Unknown";
    case CommandType::AllocateBuffer:
        return "AllocateBuffer";
    case CommandType::BindTarget:
        return "BindTarget";
    case CommandType::BindBuffer:
        return "BindBuffer";
    case CommandType::BindShader:
        return "BindShader";
    case CommandType::ApplyInstanceValue:
        return "ApplyInstanceValue";
    case CommandType::ApplyBufferValue:
        return "ApplyBufferValue";
    case CommandType::Render:
        return "Render";
    case CommandType::ApplyValue:
        return "ApplyValue";
    default:
        break;
    }
    return "";
}

QString QSSGCommand::debugString() const
{
    QString result;
    QDebug stream(&result);

    switch (m_type) {
    case CommandType::AllocateBuffer:
        static_cast<const QSSGAllocateBuffer*>(this)->addDebug(stream);
        break;
    case CommandType::BindTarget:
        static_cast<const QSSGBindTarget*>(this)->addDebug(stream);
        break;
    case CommandType::BindBuffer:
        static_cast<const QSSGBindBuffer*>(this)->addDebug(stream);
        break;
    case CommandType::BindShader:
        static_cast<const QSSGBindShader*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyInstanceValue:
        static_cast<const QSSGApplyInstanceValue*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyBufferValue:
        static_cast<const QSSGApplyBufferValue*>(this)->addDebug(stream);
        break;
    case CommandType::Render:
        static_cast<const QSSGRender*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyValue:
        static_cast<const QSSGApplyValue*>(this)->addDebug(stream);
        break;
    case CommandType::Unknown:
    default:
        addDebug(stream);
        break;
    }

    return result;
}

void QSSGCommand::addDebug(QDebug &stream) const
{
    stream << "No debug info for " << typeAsString();
}
