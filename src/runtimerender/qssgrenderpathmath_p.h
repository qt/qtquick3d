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

#ifndef QSSG_RENDER_PATH_MATH_H
#define QSSG_RENDER_PATH_MATH_H

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

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma warning(disable : 4267)
#endif

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#include <QtCore/QVector>

#include <QtQuick3DUtils/private/qssgoption_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderpathmanager_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace path {
// Solve quadratic equation.
inline int quadratic(float b, float c, float rts[2])
{
    int nquad;
    float dis;
    float rtdis;

    dis = b * b - 4 * c;
    rts[0] = 0;
    rts[1] = 0;
    if (qFuzzyIsNull(b)) {
        if (qFuzzyIsNull(c)) {
            nquad = 2;
        } else {
            if (c < 0) {
                nquad = 2;
                rts[0] = std::sqrt(-c);
                rts[1] = -rts[0];
            } else {
                nquad = 0;
            }
        }
    } else if (qFuzzyIsNull(c)) {
        nquad = 2;
        rts[0] = -b;
    } else if (dis >= 0) {
        nquad = 2;
        rtdis = std::sqrt(dis);
        if (b > 0)
            rts[0] = (-b - rtdis) * (1 / float(2));
        else
            rts[0] = (-b + rtdis) * (1 / float(2));
        if (qFuzzyIsNull(rts[0]))
            rts[1] = -b;
        else
            rts[1] = c / rts[0];
    } else {
        nquad = 0;
    }

    return (nquad);
} /* quadratic */

static const float interest_range[2] = { 0, 1 };

static void cubicInflectionPoint(const QVector2D cp[4], QVector<float> &key_point)
{
    // Convert control points to cubic monomial polynomial coefficients
    const QVector2D A = cp[3] - cp[0] + (cp[1] - cp[2]) * 3.0;
    const QVector2D B = (cp[0] - cp[1] * 2.0 + cp[2]) * 3.0, C = (cp[1] - cp[0]) * 3.0;
    const QVector2D D = cp[0];

    float a = 3 * (B.x() * A.y() - A.x() * B.y());
    float b = 3 * (C.x() * A.y() - C.y() * A.x());
    float c = C.x() * B.y() - C.y() * B.x();

    float roots[2];
    int solutions;
    // Is the quadratic really a degenerate line?
    if (qFuzzyIsNull(a)) {
        // Is the line really a degenerate point?
        if (qFuzzyIsNull(b)) {
            solutions = 0;
        } else {
            solutions = 1;
            roots[0] = c / b;
        }
    } else {
        solutions = path::quadratic(b / a, c / a, roots);
    }
    for (int i = 0; i < solutions; i++) {
        float t = static_cast<float>(roots[i]);

        QVector2D p = ((A * t + B) * t + C) * t + D;
        (void)p; // TODO: ?
        if (t >= interest_range[0] && t <= interest_range[1])
            key_point.push_back(t);
        // else; Outside range of interest, ignore.
    }
}

typedef enum { CT_POINT, CT_LINE, CT_QUADRATIC, CT_CUSP, CT_LOOP, CT_SERPENTINE } CurveType;

static inline bool isZero(double v)
{
#if 0
    const double eps = 6e-008;

    if (fabs(v) < eps)
        return true;
    else
        return false;
#else
    return qFuzzyIsNull(v);
#endif
}

static inline QVector3D crossv1(const QVector2D &a, const QVector2D &b)
{
    return QVector3D(a[1] - b[1], b[0] - a[0], a[0] * b[1] - a[1] * b[0]);
}

// static inline bool sameVertex(const QVector2D &a, const QVector2D &b)
//{
//    return (qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y()));
//}

static inline bool sameVertex(const QVector3D &a, const QVector3D &b)
{
    return (qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(a.y(), b.y()) && qFuzzyCompare(a.z(), b.z()));
}

// This function "normalizes" the input vector so the larger of its components
// is in the range [512,1024]. Exploit integer math on the exponent bits to
// do this without expensive DP exponentiation.
static inline void scaleTo512To1024(QVector2D &d, int e)
{
    union {
        quint64 u64;
        double f64;
    } x;
    int ie = 10 - e + 1023;
    Q_ASSERT(ie > 0);
    x.u64 = quint64(ie) << 52;
    d *= static_cast<float>(x.f64);
}

static inline double fastfrexp(double d, int *exponent)
{
    union {
        quint64 u64;
        double f64;
    } x;
    x.f64 = d;
    *exponent = ((int(x.u64 >> 52)) & 0x7ff) - 0x3ff;
    x.u64 &= (1ULL << 63) - (1ULL << 52);
    x.u64 |= (0x3ffULL << 52);
    return x.f64;
}

static inline QVector3D createVec3(QVector2D xy, float z)
{
    return QVector3D(xy.x(), xy.y(), z);
}

static inline QVector2D getXY(const QVector3D &data)
{
    return QVector2D(data.x(), data.y());
}

static inline CurveType cubicDoublePoint(const QVector2D points[4], QVector<float> &key_point)
{
#if 0
    const QVector2D AA = points[3] - points[0] + (points[1] - points[2]) * 3.0;
    const QVector3D BB = (points[0] - points[1] * 2.0 + points[2]) * 3.0;
    const QVector3D CC = (points[1] - points[0]) * 3.0, DD = points[0];
#endif

    // Assume control points of the cubic curve are A, B, C, and D.
    const QVector3D A = createVec3(points[0], 1);
    const QVector3D B = createVec3(points[1], 1);
    const QVector3D C = createVec3(points[2], 1);
    const QVector3D D = createVec3(points[3], 1);

    // Compute the discriminant of the roots of
    // H(s,t) = -36*(d1^2*s^2 - d1*d2*s*t + (d2^2 - d1*d3)*t^2)
    // where H is the Hessian (the square matrix of second-order
    // partial derivatives of a function) of I(s,t)
    // where I(s,t) determine the inflection points of the cubic
    // Bezier curve C(s,t).
    //
    // d1, d2, and d3 functions of the determinants constructed
    // from the cubic control points.
    //
    // Recall dot(a,cross(b,c)) is determinant of a 3x3 matrix
    // with a, b, c the rows of the matrix.
    const QVector3D DC = crossv1(getXY(D), getXY(C));
    const QVector3D AD = crossv1(getXY(A), getXY(D));
    const QVector3D BA = crossv1(getXY(B), getXY(A));

    const double a1 = QVector3D::dotProduct(A, DC);
    const double a2 = QVector3D::dotProduct(B, AD);
    const double a3 = QVector3D::dotProduct(C, BA);
    const double d1 = a1 - 2 * a2 + 3 * a3;
    const double d2 = -a2 + 3 * a3;
    const double d3 = 3 * a3;
    const double discriminant = (3 * d2 * d2 - 4 * d1 * d3);

    // The sign of the discriminant of I classifies the curbic curve
    // C into one of 6 classifications:
    // 1) discriminant>0 ==> serpentine
    // 2) discriminant=0 ==> cusp
    // 3) discriminant<0 ==> loop

    // If the discriminant or d1 are almost but not exactly zero, the
    // result is really noisy unacceptable (k,l,m) interpolation.
    // If it looks almost like a quadratic or linear case, treat it that way.
    if (isZero(discriminant) && isZero(d1)) {
        // Cusp case

        if (isZero(d2)) {
            // degenerate cases (points, lines, quadratics)...
            if (isZero(d3)) {
                if (sameVertex(A, B) && sameVertex(A, C) && sameVertex(A, D))
                    return CT_POINT;
                else
                    return CT_LINE;
            } else {
                return CT_QUADRATIC;
            }
        } else {
            return CT_CUSP;
        }
    } else if (discriminant < 0) {
        // Loop case

        const float t = static_cast<float>(d2 + sqrt(-discriminant));
        QVector2D d = QVector2D(t, static_cast<float>(2 * d1));
        QVector2D e = QVector2D(static_cast<float>(2 * (d2 * d2 - d1 * d3)), static_cast<float>(d1 * double(t)));

        // There is the situation where r2=c/t results in division by zero, but
        // in this case, the two roots represent a double root at zero so
        // subsitute l for (the otherwise NaN) m in this case.
        //
        // This situation can occur when the 1st and 2nd (or 3rd and 4th?)
        // control point of a cubic Bezier path SubPath are identical.
        if (qFuzzyIsNull(e.x()) && qFuzzyIsNull(e.y()))
            e = d;

        // d, e, or both could be very large values.  To mitigate the risk of
        // floating-point overflow in subsequent calculations
        // scale both vectors to be in the range [768,1024] since their relative
        // scale of their x & y components is irrelevant.

        // Be careful to divide by a power-of-two to disturb mantissa bits.

        double d_max_mag = qMax(qAbs(double(d.x())), qAbs(double(d.y())));
        int exponent;
        fastfrexp(d_max_mag, &exponent);
        scaleTo512To1024(d, exponent);

        double e_max_mag = qMax(qAbs(double(e.x())), qAbs(double(e.y())));
        fastfrexp(e_max_mag, &exponent);
        scaleTo512To1024(e, exponent);

        const QVector2D roots = QVector2D(d.x() / d.y(), e.x() / e.y());

        double tt;
#if 0
        tt = roots[0];
        if (tt >= interest_range[0] && tt <= interest_range[1])
            // key_point.push_back(tt);
            tt = roots[1];
        if (tt >= interest_range[0] && tt <= interest_range[1])
            // key_point.push_back(tt);
#endif
        tt = (double(roots[0] + roots[1])) / 2.0;
        if (tt >= double(interest_range[0]) && tt <= double(interest_range[1]))
            key_point.push_back(static_cast<float>(tt));

        return CT_LOOP;
    } else {
        Q_ASSERT(discriminant >= 0);
        cubicInflectionPoint(points, key_point);
        if (discriminant > 0) {
            // Serpentine case
            return CT_SERPENTINE;
        } else {
            // Cusp with inflection at infinity (treat like serpentine)
            return CT_CUSP;
        }
    }
}

inline QVector4D createVec4(QVector2D p1, QVector2D p2)
{
    return QVector4D(p1.x(), p1.y(), p2.x(), p2.y());
}

static inline QVector2D lerp(QVector2D p1, QVector2D p2, float distance)
{
    return p1 + (p2 - p1) * distance;
}

static inline float lerp(float p1, float p2, float distance)
{
    return p1 + (p2 - p1) * distance;
}

// Using first derivative to get tangent.
// If this equation does not make immediate sense consider that it is the first derivative
// of the de Casteljau bezier expansion, not the polynomial expansion.
static inline float tangentAt(float inT, float p1, float c1, float c2, float p2)
{
    float a = c1 - p1;
    float b = c2 - c1 - a;
    float c = p2 - c2 - a - (2.0f * b);
    float retval = 3.0f * (a + (2.0f * b * inT) + (c * inT * inT));
    return retval;
}

// static inline QVector2D midpoint(QVector2D p1, QVector2D p2)
//{
//    return lerp(p1, p2, .5f);
//}

static inline float lineLength(QVector2D inStart, QVector2D inStop)
{
    const QVector2D diff = inStop - inStart;
    return ::sqrtf(diff.x() * diff.x() + diff.y() * diff.y());
}

struct QSSGCubicBezierCurve
{
    QVector2D m_points[4];
    QSSGCubicBezierCurve(QVector2D a1, QVector2D c1, QVector2D c2, QVector2D a2)
    {
        m_points[0] = a1;
        m_points[1] = c1;
        m_points[2] = c2;
        m_points[3] = a2;
    }

    // Normal is of course orthogonal to the tangent.
    QVector2D normalAt(float inT) const
    {
        QVector2D tangent = QVector2D(tangentAt(inT, m_points[0].x(), m_points[1].x(), m_points[2].x(), m_points[3].x()),
                                      tangentAt(inT, m_points[0].y(), m_points[1].y(), m_points[2].y(), m_points[3].y()));

        QVector2D result(tangent.y(), -tangent.x());
        result.normalize();
        return result;
    }

    QPair<QSSGCubicBezierCurve, QSSGCubicBezierCurve> splitCubicBezierCurve(float inT)
    {
        // compute point on curve based on inT
        // using de Casteljau algorithm
        QVector2D p12 = lerp(m_points[0], m_points[1], inT);
        QVector2D p23 = lerp(m_points[1], m_points[2], inT);
        QVector2D p34 = lerp(m_points[2], m_points[3], inT);
        QVector2D p123 = lerp(p12, p23, inT);
        QVector2D p234 = lerp(p23, p34, inT);
        QVector2D p1234 = lerp(p123, p234, inT);

        return QPair<QSSGCubicBezierCurve, QSSGCubicBezierCurve>(QSSGCubicBezierCurve(m_points[0], p12, p123, p1234),
                                                                     QSSGCubicBezierCurve(p1234, p234, p34, m_points[3]));
    }
};

struct QSSGResultCubic
{
    enum Mode {
        Normal = 0,
        BeginTaper = 1,
        EndTaper = 2,
    };
    QVector2D m_p1;
    QVector2D m_c1;
    QVector2D m_c2;
    QVector2D m_p2;
    // Location in the original data where this cubic is taken from
    quint32 m_equationIndex;
    float m_tStart;
    float m_tStop;
    float m_length;
    QVector2D m_taperMultiplier; // normally 1, goes to zero at very end of taper if any taper.
    Mode m_mode;

    QSSGResultCubic(QVector2D inP1, QVector2D inC1, QVector2D inC2, QVector2D inP2, quint32 equationIndex, float tStart, float tStop, float length)
        : m_p1(inP1)
        , m_c1(inC1)
        , m_c2(inC2)
        , m_p2(inP2)
        , m_equationIndex(equationIndex)
        , m_tStart(tStart)
        , m_tStop(tStop)
        , m_length(length)
        , m_taperMultiplier(1.0f, 1.0f)
        , m_mode(Normal)
    {
    }
    // Note the vec2 items are *not* initialized in any way here.
    QSSGResultCubic() {}
    float getP1Width(float inPathWidth, float beginTaperWidth, float endTaperWidth)
    {
        return getPathWidth(inPathWidth, beginTaperWidth, endTaperWidth, 0);
    }

    float getP2Width(float inPathWidth, float beginTaperWidth, float endTaperWidth)
    {
        return getPathWidth(inPathWidth, beginTaperWidth, endTaperWidth, 1);
    }

    float getPathWidth(float inPathWidth, float beginTaperWidth, float endTaperWidth, quint32 inTaperIndex)
    {
        float retval = inPathWidth;
        switch (m_mode) {
        case BeginTaper:
            retval = beginTaperWidth * m_taperMultiplier[inTaperIndex];
            break;
        case EndTaper:
            retval = endTaperWidth * m_taperMultiplier[inTaperIndex];
            break;
        default:
            break;
        }
        return retval;
    }
};

inline void pushLine(QVector<QSSGResultCubic> &ioResultVec, QVector2D inStart, QVector2D inStop, quint32 inEquationIndex)
{
    QVector2D range = inStop - inStart;
    ioResultVec.push_back(
            QSSGResultCubic(inStart, inStart + range * .333f, inStart + range * .666f, inStop, inEquationIndex, 0.0f, 1.0f, lineLength(inStart, inStop)));
}

enum class QSSGPathDirtyFlagValue
{
    None = 0,
    SourceData = 1,
    PathType = 1 << 1,
    Width = 1 << 2,
    BeginTaper = 1 << 3,
    EndTaper = 1 << 4,
    CPUError = 1 << 5,
};

Q_DECLARE_FLAGS(QSSGPathDirtyFlags, QSSGPathDirtyFlagValue)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGPathDirtyFlags)

struct QSSGTaperInformation
{
    float m_capOffset = 0.0f;
    float m_capOpacity = 0.0f;
    float m_capWidth = 0.0f;

    QSSGTaperInformation() = default;
    QSSGTaperInformation(float capOffset, float capOpacity, float capWidth)
        : m_capOffset(capOffset), m_capOpacity(capOpacity), m_capWidth(capWidth)
    {
    }

    bool operator==(const QSSGTaperInformation &inOther) const
    {
        return qFuzzyCompare(m_capOffset, inOther.m_capOffset) && qFuzzyCompare(m_capOpacity, inOther.m_capOpacity)
                && qFuzzyCompare(m_capWidth, inOther.m_capWidth);
    }
};

void outerAdaptiveSubdivideBezierCurve(QVector<QSSGResultCubic> &ioResultVec,
                                       QVector<float> &keyPointVec,
                                       QSSGCubicBezierCurve inCurve,
                                       float inLinearError,
                                       quint32 inEquationIndex);

static void adaptiveSubdivideBezierCurve(QVector<QSSGResultCubic> &ioResultVec,
                                         QSSGCubicBezierCurve &inCurve,
                                         float inLinearError,
                                         quint32 inEquationIndex,
                                         float inTStart,
                                         float inTStop);

// Adaptively subdivide source data to produce m_PatchData.
// static void adaptiveSubdivideSourceData(QSSGConstDataRef<QSSGPathAnchorPoint> inSourceData,
//                                        QVector<QSSGResultCubic> &ioResultVec,
//                                        QVector<float> &keyPointVec, float inLinearError)
//{
//    ioResultVec.clear();
//    if (inSourceData.size() < 2)
//        return;
//    // Assuming no attributes in the source data.
//    quint32 numEquations = (inSourceData.size() - 1);
//    for (quint32 idx = 0, end = numEquations; idx < end; ++idx) {
//        const QSSGPathAnchorPoint &beginAnchor = inSourceData[idx];
//        const QSSGPathAnchorPoint &endAnchor = inSourceData[idx + 1];

//        QVector2D anchor1(beginAnchor.position);
//        QVector2D control1(QSSGPathManagerCoreInterface::getControlPointFromAngleDistance(
//                               beginAnchor.position, beginAnchor.outgoingAngle,
//                               beginAnchor.outgoingDistance));

//        QVector2D control2(QSSGPathManagerCoreInterface::getControlPointFromAngleDistance(
//                               endAnchor.position, endAnchor.incomingAngle,
//                               endAnchor.incomingDistance));
//        QVector2D anchor2(endAnchor.position);

//        outerAdaptiveSubdivideBezierCurve(
//                    ioResultVec, keyPointVec,
//                    QSSGCubicBezierCurve(anchor1, control1, control2, anchor2), inLinearError, idx);
//    }
//}

// The outer subdivide function topologically analyzes the curve to ensure that
// the sign of the second derivative does not change, no inflection points.
// Once that condition is held, then we proceed with a simple adaptive subdivision algorithm
// until the curve is accurately approximated by a straight line.
inline void outerAdaptiveSubdivideBezierCurve(QVector<QSSGResultCubic> &ioResultVec,
                                              QVector<float> &keyPointVec,
                                              QSSGCubicBezierCurve inCurve,
                                              float inLinearError,
                                              quint32 inEquationIndex)
{
    // Step 1, find what type of curve we are dealing with and the inflection points.
    keyPointVec.clear();
    CurveType theCurveType = cubicDoublePoint(inCurve.m_points, keyPointVec);

    float tStart = 0;
    switch (theCurveType) {
    case CT_POINT:
        ioResultVec.push_back(
                QSSGResultCubic(inCurve.m_points[0], inCurve.m_points[0], inCurve.m_points[0], inCurve.m_points[0], inEquationIndex, 0.0f, 1.0f, 0.0f));
        return; // don't allow further recursion
    case CT_LINE:
        pushLine(ioResultVec, inCurve.m_points[0], inCurve.m_points[3], inEquationIndex);
        return; // don't allow further recursion
    case CT_CUSP:
    case CT_LOOP:
    case CT_SERPENTINE: {
        // Break the curve at the inflection points if there is one.  If there aren't
        // inflection points
        // the treat as linear (degenerate case that should not happen except in limiting
        // ranges of floating point accuracy)
        if (!keyPointVec.empty()) {
            // It is not clear that the code results in a sorted vector,
            // or a vector where all values are within the range of 0-1

            if (keyPointVec.size() > 1)
                std::sort(keyPointVec.begin(), keyPointVec.end());
            for (quint32 idx = 0, end = quint32(keyPointVec.size()); idx < end && keyPointVec[idx] < 1.0f; ++idx) {
                // We have a list of T values I believe sorted from beginning to end, we
                // will create a set of bezier curves
                // Since we split the curves, tValue is relative to tSTart, not 0.
                float range = 1.0f - tStart;
                float splitPoint = keyPointVec[idx] - tStart;
                float tValue = splitPoint / range;
                if (tValue > 0.0f) {
                    QPair<QSSGCubicBezierCurve, QSSGCubicBezierCurve> newCurves = inCurve.splitCubicBezierCurve(tValue);
                    adaptiveSubdivideBezierCurve(ioResultVec, newCurves.first, inLinearError, inEquationIndex, tStart, splitPoint);
                    inCurve = newCurves.second;
                    tStart = splitPoint;
                }
            }
        }
    }
    // fallthrough intentional
    break;
        // fallthrough intentional
    case CT_QUADRATIC:
        break;
    }
    adaptiveSubdivideBezierCurve(ioResultVec, inCurve, inLinearError, inEquationIndex, tStart, 1.0f);
}

static float distanceFromPointToLine(QVector2D inLineDxDy, QVector2D lineStart, QVector2D point)
{
    QVector2D pointToLineStart = lineStart - point;
    return qAbs((inLineDxDy.x() * pointToLineStart.y()) - (inLineDxDy.y() * pointToLineStart.x()));
}

// There are two options here.  The first is to just subdivide below a given error
// tolerance.
// The second is to fit a quadratic to the curve and then precisely find the length of the
// quadratic.
// Obviously we are choosing the subdivide method at this moment but I think the fitting
// method is probably more robust.
static float lengthOfBezierCurve(QSSGCubicBezierCurve &inCurve)
{
    // Find distance of control points from line.  Note that both control points should be
    // on same side of line else we have a serpentine which should have been removed by topological
    // analysis.
    QVector2D lineDxDy = inCurve.m_points[3] - inCurve.m_points[0];
    float c1Distance = distanceFromPointToLine(lineDxDy, inCurve.m_points[0], inCurve.m_points[1]);
    float c2Distance = distanceFromPointToLine(lineDxDy, inCurve.m_points[0], inCurve.m_points[2]);
    const float lineTolerance = 100.0f; // error in world coordinates, squared.
    if (c1Distance > lineTolerance || c2Distance > lineTolerance) {
        QPair<QSSGCubicBezierCurve, QSSGCubicBezierCurve> subdivCurve = inCurve.splitCubicBezierCurve(.5f);
        return lengthOfBezierCurve(subdivCurve.first) + lengthOfBezierCurve(subdivCurve.second);
    } else {
        return lineLength(inCurve.m_points[0], inCurve.m_points[3]);
    }
}

// The assumption here is the the curve type is not cusp, loop, or serpentine.
// It is either linear or it is a constant curve meaning we can use very simple means to
// figure out the curvature.  There is a possibility to use some math to figure out the point of
// maximum curvature, where the second derivative will have a max value. This is probably not
// necessary.
static void adaptiveSubdivideBezierCurve(QVector<QSSGResultCubic> &ioResultVec,
                                         QSSGCubicBezierCurve &inCurve,
                                         float inLinearError,
                                         quint32 inEquationIndex,
                                         float inTStart,
                                         float inTStop)
{
    // Find distance of control points from line.  Note that both control points should be
    // on same side of line else we have a serpentine which should have been removed by topological
    // analysis.
    QVector2D lineDxDy = inCurve.m_points[3] - inCurve.m_points[0];
    float c1Distance = distanceFromPointToLine(lineDxDy, inCurve.m_points[0], inCurve.m_points[1]);
    float c2Distance = distanceFromPointToLine(lineDxDy, inCurve.m_points[0], inCurve.m_points[2]);
    const float lineTolerance = inLinearError * inLinearError; // error in world coordinates
    if (c1Distance > lineTolerance || c2Distance > lineTolerance) {
        QPair<QSSGCubicBezierCurve, QSSGCubicBezierCurve> subdivCurve = inCurve.splitCubicBezierCurve(.5f);
        float halfway = lerp(inTStart, inTStop, .5f);
        adaptiveSubdivideBezierCurve(ioResultVec, subdivCurve.first, inLinearError, inEquationIndex, inTStart, halfway);
        adaptiveSubdivideBezierCurve(ioResultVec, subdivCurve.second, inLinearError, inEquationIndex, halfway, inTStop);
    } else {
        ioResultVec.push_back(QSSGResultCubic(inCurve.m_points[0],
                                                inCurve.m_points[1],
                                                inCurve.m_points[2],
                                                inCurve.m_points[3],
                                                inEquationIndex,
                                                inTStart,
                                                inTStop,
                                                lengthOfBezierCurve(inCurve)));
    }
}
}
QT_END_NAMESPACE

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma warning(default : 4267)
#endif
#endif
