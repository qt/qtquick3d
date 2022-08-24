// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGUTILS_H
#define QSSGUTILS_H

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
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>
#include <QtGui/QMatrix3x3>
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>

#include <QtCore/qdebug.h>
#include <QtCore/QString>
#include <QtCore/qloggingcategory.h>
#include <QtCore/QIODevice>
#include <QtCore/qmath.h>

class tst_RotationDataClass;

QT_BEGIN_NAMESPACE

namespace aux {
Q_DECL_CONSTEXPR inline float translateConstantAttenuation(float attenuation) { return attenuation; }
template<int MINATTENUATION = 0, int MAXATTENUATION = 1000>
Q_DECL_CONSTEXPR inline float translateLinearAttenuation(float attenuation) { return qBound(float(MINATTENUATION), attenuation, float(MAXATTENUATION)) * .01f; }
template<int MINATTENUATION = 0, int MAXATTENUATION = 1000>
Q_DECL_CONSTEXPR inline float translateQuadraticAttenuation(float attenuation) { return qBound(float(MINATTENUATION), attenuation, float(MAXATTENUATION)) * .0001f; }
}

namespace vec2 {
float Q_QUICK3DUTILS_EXPORT magnitude(const QVector2D &v);
}

namespace vec3 {
inline QVector3D minimum(const QVector3D &v1, const QVector3D &v2) Q_DECL_NOTHROW { return { qMin(v1.x(), v2.x()), qMin(v1.y(), v2.y()), qMin(v1.z(), v2.z()) }; }
inline QVector3D maximum(const QVector3D &v1, const QVector3D &v2) Q_DECL_NOTHROW { return { qMax(v1.x(), v2.x()), qMax(v1.y(), v2.y()), qMax(v1.z(), v2.z()) }; }
bool Q_QUICK3DUTILS_EXPORT isFinite(const QVector3D &v);
float Q_QUICK3DUTILS_EXPORT magnitude(const QVector3D &v);
float Q_QUICK3DUTILS_EXPORT magnitudeSquared(const QVector3D &v);
float Q_QUICK3DUTILS_EXPORT normalize(QVector3D &v);
}

namespace mat33 {
QVector3D Q_QUICK3DUTILS_EXPORT transform(const QMatrix3x3 &m, const QVector3D &v);
}

namespace mat44 {
QMatrix3x3 Q_QUICK3DUTILS_EXPORT getUpper3x3(const QMatrix4x4 &m);
void Q_QUICK3DUTILS_EXPORT normalize(QMatrix4x4 &m);
QVector3D Q_QUICK3DUTILS_EXPORT rotate(const QMatrix4x4 &m, const QVector3D &v);
QVector4D Q_QUICK3DUTILS_EXPORT rotate(const QMatrix4x4 &m, const QVector4D &v);
QVector3D Q_QUICK3DUTILS_EXPORT transform(const QMatrix4x4 &m, const QVector3D &v);
QVector4D Q_QUICK3DUTILS_EXPORT transform(const QMatrix4x4 &m, const QVector4D &v);
QVector3D Q_QUICK3DUTILS_EXPORT getPosition(const QMatrix4x4 &m);
QVector3D Q_QUICK3DUTILS_EXPORT getScale(const QMatrix4x4 &m);

inline void flip(QMatrix4x4 &matrix)
{
    // Flip between left-handed and right-handed orientation
    float *writePtr(matrix.data());
    // rotation conversion
    writePtr[0 * 4 + 2] *= -1;
    writePtr[1 * 4 + 2] *= -1;
    writePtr[2 * 4 + 0] *= -1;
    writePtr[2 * 4 + 1] *= -1;
    // translation conversion
    writePtr[3 * 4 + 2] *= -1;
}

}

namespace quant {
bool Q_QUICK3DUTILS_EXPORT isFinite(const QQuaternion &q);

float Q_QUICK3DUTILS_EXPORT magnitude(const QQuaternion &q);

bool Q_QUICK3DUTILS_EXPORT isSane(const QQuaternion &q);

bool Q_QUICK3DUTILS_EXPORT isUnit(const QQuaternion &q);

QVector3D Q_QUICK3DUTILS_EXPORT rotated(const QQuaternion &q, const QVector3D &v);

QVector3D Q_QUICK3DUTILS_EXPORT inverseRotated(const QQuaternion &q, const QVector3D &v);
}

namespace color {
QVector4D Q_QUICK3DUTILS_EXPORT sRGBToLinear(const QColor &color);
}

template<typename TDataType>
QSSGDataRef<TDataType> PtrAtOffset(quint8 *baseData, quint32 offset, quint32 byteSize)
{
    return QSSGDataRef<TDataType>(byteSize ? reinterpret_cast<TDataType *>(baseData + offset) : nullptr,
                                    byteSize / sizeof(TDataType));
}

Q_QUICK3DUTILS_EXPORT const char *nonNull(const char *src);

inline QVector3D degToRad(const QVector3D &v) {
    return QVector3D(qDegreesToRadians(v.x()), qDegreesToRadians(v.y()), qDegreesToRadians(v.z()));
}

inline QVector3D radToDeg(const QVector3D &v) {
    return QVector3D(qRadiansToDegrees(v.x()), qRadiansToDegrees(v.y()), qRadiansToDegrees(v.z()));
}

class RotationData
{
public:
    RotationData() = default;
    explicit RotationData(const QVector3D &r)
        : m_quatRot()
        , m_eulerRot(r)
        , m_dirty(Dirty::Quaternion)
    {}
    explicit RotationData(const QQuaternion &r)
        : m_quatRot(r.normalized())
        , m_eulerRot()
        , m_dirty(Dirty::Euler)
    {}

    RotationData &operator=(const QVector3D &r) noexcept
    {
        m_eulerRot = r;
        m_dirty = Dirty::Quaternion;
        return *this;
    }
    RotationData &operator=(const QQuaternion &r) noexcept
    {
        m_quatRot = r.normalized();
        m_dirty = Dirty::Euler;
        return *this;
    }

    friend inline bool operator ==(const RotationData &a, const RotationData &b) {
        if (a.m_dirty == Dirty::None && b.m_dirty == Dirty::None)
            return fuzzyQuaternionCompare(a.m_quatRot, b.m_quatRot);

        return fuzzyQuaternionCompare(QQuaternion(a), QQuaternion(b));
    }

    friend inline bool operator !=(const RotationData &a, const RotationData &b) { return !(a == b); }

    friend inline bool operator ==(const RotationData &a, const QVector3D &eulerRotation)
    {
        if (a.m_dirty == Dirty::None)
            return qFuzzyCompare(a.m_eulerRot, eulerRotation);

        return qFuzzyCompare(QVector3D(a), eulerRotation);
    }
    friend inline bool operator !=(const RotationData &a, const QVector3D &eulerRotation) { return !(a == eulerRotation); }

    friend inline bool operator ==(const RotationData &a, const QQuaternion &rotation)
    {
        if (a.m_dirty == Dirty::None)
            return fuzzyQuaternionCompare(a.m_quatRot, rotation);

        return fuzzyQuaternionCompare(QQuaternion(a), rotation);
    }
    friend inline bool operator !=(const RotationData &a, const QQuaternion &rotation) { return !(a == rotation); }

    [[nodiscard]] inline QVector3D getEulerRotation() const
    {
        if (m_dirty == Dirty::Euler) {
            m_eulerRot = m_quatRot.toEulerAngles();
            m_dirty = Dirty::None;
        }

        return m_eulerRot;
    }

    [[nodiscard]] inline QQuaternion getQuaternionRotation() const
    {
        if (m_dirty == Dirty::Quaternion) {
            m_quatRot = QQuaternion::fromEulerAngles(m_eulerRot).normalized();
            m_dirty = Dirty::None;
        }

        return m_quatRot;
    }

    [[nodiscard]] inline QMatrix3x3 toRotationMatrix() const { return getQuaternionRotation().toRotationMatrix(); }

    [[nodiscard]] inline operator QQuaternion() const { return getQuaternionRotation(); }
    [[nodiscard]] inline operator QVector3D() const { return getEulerRotation(); }

private:
    friend class ::tst_RotationDataClass;

    [[nodiscard]] constexpr static inline bool fuzzyQuaternionCompare(const QQuaternion &a, const QQuaternion &b)
    {
        // This 'e' will give better precision than qtbase's qFuzzyCompare for QQuaternion
        constexpr float e = 0.0000001f;
        return (qAbs(1.0f - qAbs(QQuaternion::dotProduct(a, b))) <= e);
    }

    enum class Dirty
    {
        None,
        Quaternion = 0x1,
        Euler = 0x2
    };

    mutable QQuaternion m_quatRot; // Should always be normalized
    mutable QVector3D m_eulerRot;
    mutable Dirty m_dirty { Dirty::None };
};

QT_END_NAMESPACE

#endif // QSSGUTILS_H
