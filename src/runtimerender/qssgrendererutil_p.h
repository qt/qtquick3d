/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
        : m_path(p), m_key(qHash(p, QHashSeed::globalSeed())) {}

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
