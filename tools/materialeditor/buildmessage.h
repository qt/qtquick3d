/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef BUILDMESSAGE_H
#define BUILDMESSAGE_H

#include <QtCore/qobject.h>
#include <QtCore/qdebug.h>
#include <QtQml/qqmlregistration.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

QT_BEGIN_NAMESPACE

struct BuildMessage
{
    using Status = QtQuick3DEditorHelpers::ShaderBaker::Status;
    QString message;
    QString identifier;
    qint64 line = -1;
    qint64 column = -1;
    Status status = Status::Success;

    friend QDebug operator<<(QDebug stream, const BuildMessage &err)
    {
        stream << err.message;
        if (err.line != -1)
            stream << "at line: " << err.line;
        return stream;
    }
};

class ShaderBuildMessage
{
    Q_GADGET
    Q_PROPERTY(QString filename READ filename)
    Q_PROPERTY(QString message READ message)
    Q_PROPERTY(QString identifier READ identifier)
    Q_PROPERTY(qint64 line READ line)
    Q_PROPERTY(Status status READ status)
    Q_PROPERTY(Stage stage READ stage)
    QML_VALUE_TYPE(shaderStatus)
public:
    enum class Status
    {
        Success,
        Error
    };
    Q_ENUM(Status)
    enum class Stage
    {
        Vertex,
        Fragment
    };
    Q_ENUM(Stage)

    ShaderBuildMessage() = default;
    ShaderBuildMessage(const BuildMessage &data, const QString &filename, Stage stage);
    const QString &filename() const { return m_filename; }
    const QString &message() const { return m_message.message; }
    const QString &identifier() const { return m_message.identifier; }
    qint64 line() const { return m_message.line; }
    Status status() const { return static_cast<Status>(m_message.status); }
    Stage stage() const { return m_stage; }

private:
    BuildMessage m_message;
    QString m_filename;
    Stage m_stage;
};

namespace ShaderEnums {
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(ShaderBuildMessage)
QML_NAMED_ELEMENT(ShaderConstants)
}

QT_END_NAMESPACE

#endif // BUILDMESSAGE_H
