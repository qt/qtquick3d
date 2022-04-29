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
