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

#ifndef QSSGPATHUTILITIES_H
#define QSSGPATHUTILITIES_H

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

#include <QtQuick3DAssetImport/private/qtquick3dassetimportglobal_p.h>

#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtCore/QIODevice>
#include <QtCore/QSharedPointer>
#include <QtGui/QVector2D>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

namespace QSSGPathUtilities {

enum class PathCommand : quint8
{
    None = 0,
    MoveTo, // 2 floats
    CubicCurveTo, // 6 floats, c1, c2, p2.  p1 is existing location
    Close, // 0 floats
};

struct Q_QUICK3DASSETIMPORT_EXPORT QSSGPathBuffer
{
    // 64 bit random number to uniquely identify this file type.
    static quint64 getFileTag() { return 0x7b1a41633c43a6afULL; }
    static quint32 getFileVersion() { return 1; }
    QSSGDataView<PathCommand> commands;
    QSSGDataView<float> data;
    QSSGPathBuffer() = default;
    void save(QIODevice &outStream) const;
    static QSSGPathBuffer *load(QIODevice &inStream);
};

struct Q_QUICK3DASSETIMPORT_EXPORT QSSGPathBufferBuilder
{
    QVector<PathCommand> m_commands;
    QVector<float> m_data;

    void clear()
    {
        m_commands.clear();
        m_data.clear();
    }

    void push(const QVector2D &inPos)
    {
        m_data.push_back(inPos.x());
        m_data.push_back(inPos.y());
    }

    void moveTo(const QVector2D &inPos)
    {
        m_commands.push_back(PathCommand::MoveTo);
        push(inPos);
    }

    void cubicCurveTo(const QVector2D &inC1, const QVector2D &inC2, const QVector2D &inP2)
    {
        m_commands.push_back(PathCommand::CubicCurveTo);
        push(inC1);
        push(inC2);
        push(inP2);
    }

    void close() { m_commands.push_back(PathCommand::Close); }

    // Points back to internal data structures, must use or copy.
    QSSGPathBuffer getPathBuffer()
    {
        QSSGPathBuffer retval;
        retval.data = toDataView(m_data);
        retval.commands = toDataView(m_commands);
        ;
        return retval;
    }
};

} // end namespace

QT_END_NAMESPACE

#endif // QSSGPATHUTILITIES_H
