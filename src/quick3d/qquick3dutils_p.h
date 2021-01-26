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

#ifndef QQUICK3DUTILS_P_H
#define QQUICK3DUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <type_traits>

#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE

// Assigns 'updated' to 'orig' and returns true if they are different
template<typename T, typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
bool qUpdateIfNeeded(T &orig, T updated)
{
    if (orig == updated)
        return false;
    orig = updated;
    return true;
}

// Assigns 'updated' to 'orig' and returns true if they are different, compared with qFuzzyCompare
template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
bool qUpdateIfNeeded(T &orig, T updated)
{
    if (qFuzzyCompare(orig, updated))
        return false;
    orig = updated;
    return true;
}

QT_END_NAMESPACE

#endif // QQUICK3DUTILS_P_H
