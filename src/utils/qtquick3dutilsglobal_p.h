// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTSSGGLOBAL_P_H
#define QTSSGGLOBAL_P_H

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

#include <QtCore/qglobal.h>
#include <QtQuick3DUtils/private/qtquick3dutilsexports_p.h>

QT_BEGIN_NAMESPACE

template<typename T>
class QSSGRef
{
    T *d;

public:
    T *data() const { return d; }
    T *get() const { return d; }
    T *take()
    {
        T *t = d;
        d = nullptr;
        return t;
    }
    bool isNull() const { return !d; }
    operator bool() const { return d; }
    bool operator!() const { return !d; }
    T &operator*() const { return *d; }
    T *operator->() const { return d; }

    // constructors
    constexpr QSSGRef() : d(nullptr) {}
    constexpr QSSGRef(std::nullptr_t) : d(nullptr) {}

    QSSGRef(T *ptr) : d(ptr)
    {
        if (d)
            d->ref.ref();
    }
    template<typename X>
    QSSGRef(X *ptr) : d(ptr)
    {
        if (d)
            d->ref.ref();
    }
    QSSGRef(const QSSGRef<T> &other) : d(other.d)
    {
        if (d)
            d->ref.ref();
    }
    template<typename X>
    QSSGRef(const QSSGRef<X> &other) : d(other.get())
    {
        if (d)
            d->ref.ref();
    }

    QSSGRef(QSSGRef<T> &&other) noexcept : d(other.take()) {}
    template<typename X>
    QSSGRef(QSSGRef<X> &&other) noexcept : d(other.take())
    {
    }

    ~QSSGRef()
    {
        if (d && !d->ref.deref())
            delete d;
    }

    template<typename X>
    QSSGRef<T> &operator=(const QSSGRef<X> &other)
    {
        if (d != other.get()) {
            if (d && !d->ref.deref())
                delete d;
            d = other.get();
            if (d)
                d->ref.ref();
        }
        return *this;
    }
    template<typename X>
    QSSGRef<T> &operator=(QSSGRef<X> &&other) noexcept
    {
        clear();
        d = other.take();
        return *this;
    }
    QSSGRef<T> &operator=(const QSSGRef<T> &other)
    {
        if (d != other.get()) {
            if (d && !d->ref.deref())
                delete d;
            d = other.get();
            if (d)
                d->ref.ref();
        }
        return *this;
    }
    QSSGRef<T> &operator=(QSSGRef<T> &&other) noexcept
    {
        qSwap(d, other.d);
        return *this;
    }

    void swap(QSSGRef<T> &other) { qSwap(d, other.d); }

    void clear()
    {
        if (d && !d->ref.deref())
            delete d;
        d = nullptr;
    }
};

template<class T, class X>
bool operator==(const QSSGRef<T> &ptr1, const QSSGRef<X> &ptr2)
{
    return ptr1.get() == ptr2.get();
}
template<class T, class X>
bool operator!=(const QSSGRef<T> &ptr1, const QSSGRef<X> &ptr2)
{
    return ptr1.get() != ptr2.get();
}
template<class T, class X>
bool operator==(const QSSGRef<T> &ptr1, const X *ptr2)
{
    return ptr1.get() == ptr2;
}
template<class T, class X>
bool operator!=(const QSSGRef<T> &ptr1, const X *ptr2)
{
    return ptr1.get() != ptr2;
}
template<class T, class X>
bool operator==(const T *ptr1, const QSSGRef<X> &ptr2)
{
    return ptr1 == ptr2.get();
}
template<class T, class X>
bool operator!=(const T *ptr1, const QSSGRef<X> &ptr2)
{
    return ptr1 != ptr2.get();
}
template<class T>
bool operator==(const QSSGRef<T> &lhs, std::nullptr_t)
{
    return !lhs.get();
}
template<class T>
bool operator!=(const QSSGRef<T> &lhs, std::nullptr_t)
{
    return lhs.get();
}
template<class T>
bool operator==(std::nullptr_t, const QSSGRef<T> &rhs)
{
    return !rhs.get();
}
template<class T>
bool operator!=(std::nullptr_t, const QSSGRef<T> &rhs)
{
    return rhs.get();
}

QT_END_NAMESPACE

#endif // QTSSGGLOBAL_P_H
