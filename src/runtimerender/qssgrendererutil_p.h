/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDERER_UTIL_H
#define QSSG_RENDERER_UTIL_H

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

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

namespace QSSGRendererUtil
{
inline constexpr quint32 nextMultipleOf4(quint32 value) {
    return (value + 3) & ~3;
}
}

class QSSGRenderPath
{
public:
    QSSGRenderPath() = default;
    explicit inline QSSGRenderPath(const QString &p) noexcept
        : m_path(p), m_key(qHash(p, qGlobalQHashSeed())) {}

    inline bool isNull() const { return m_path.isNull(); }
    inline bool isEmpty() const { return m_path.isEmpty(); }
    QString path() const { return m_path; }
private:
    friend bool operator==(const QSSGRenderPath &, const QSSGRenderPath &);
    friend size_t qHash(const QSSGRenderPath &, size_t) Q_DECL_NOTHROW;
    QString m_path;
    size_t m_key = 0;
};

inline bool operator==(const QSSGRenderPath &p1, const QSSGRenderPath &p2)
{
    return (p1.m_key == p2.m_key) && (p1.m_path == p2.m_path);
}

inline size_t qHash(const QSSGRenderPath &path, size_t seed) Q_DECL_NOTHROW
{
    return (path.m_key) ? path.m_key : qHash(path.m_path, seed);
}

QT_END_NAMESPACE

#endif
