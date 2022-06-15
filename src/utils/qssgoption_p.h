// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGOption_H
#define QSSGOption_H

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

#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>

QT_BEGIN_NAMESPACE

struct QSSGEmpty
{
};

template<typename TDataType>
class QSSGOption
{
    TDataType mData;
    bool mHasValue;

public:
    QSSGOption(const TDataType &data) : mData(data), mHasValue(true) {}
    QSSGOption(const QSSGEmpty &) : mHasValue(false) {}
    QSSGOption() : mHasValue(false) {}
    QSSGOption(const QSSGOption &other) : mData(other.mData), mHasValue(other.mHasValue) {}
    QSSGOption &operator=(const QSSGOption &other)
    {
        mData = other.mData;
        mHasValue = other.mHasValue;
        return *this;
    }

    bool isEmpty() const { return !mHasValue; }
    void setEmpty() { mHasValue = false; }
    bool hasValue() const { return mHasValue; }

    const TDataType &getValue() const
    {
        Q_ASSERT(mHasValue);
        return mData;
    }
    TDataType &getValue()
    {
        Q_ASSERT(mHasValue);
        return mData;
    }
    TDataType &unsafeGetValue() { return mData; }

    operator const TDataType &() const { return getValue(); }
    operator TDataType &() { return getValue(); }

    const TDataType *operator->() const { return &getValue(); }
    TDataType *operator->() { return &getValue(); }

    const TDataType &operator*() const { return getValue(); }
    TDataType &operator*() { return getValue(); }

    friend bool operator==(const QSSGOption &a, const QSSGOption &b) {
        return a.mHasValue == b.mHasValue && a.mData == b.mData;
    }
    friend bool operator!=(const QSSGOption &a, const QSSGOption &b) {
        return !(a == b);
    }
};

QT_END_NAMESPACE

#endif // QSSGOption_H
