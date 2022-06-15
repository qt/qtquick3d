// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include <private/qglobal_p.h>

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
