/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "qssgrendereulerangles_p.h"

#include <cmath>
#include <cfloat>
#include <cstring>
#include <cstdio>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#ifdef _MSC_VER
#pragma warning(disable : 4365) // warnings on conversion from unsigned int to int
#endif

QT_BEGIN_NAMESPACE

//==============================================================================
/**
 *	Constructor
 */
QSSGEulerAngleConverter::QSSGEulerAngleConverter()
{
    m_orderInfoBuffer[0] = '\0';
}

//==============================================================================
/**
 *	Destructor
 */
QSSGEulerAngleConverter::~QSSGEulerAngleConverter() = default;

//==============================================================================
/**
 *  Constructs a Euler angle & holds it in a EulerAngles struct
 *  @param theI         x rotation ( radians )
 *  @param theJ         y rotation ( radians )
 *  @param theH         z rotation ( radians )
 *  @param theOrder     the order this angle is in namely XYZ( static ), etc.
 *                      use the getEulerOrder() to generate this enum
 *  @return the euler angle
 */
EulerAngles QSSGEulerAngleConverter::euler(float theI, float theJ, float theH, EulerOrder theOrder)
{
    EulerAngles theEulerAngle;
    theEulerAngle.x = theI;
    theEulerAngle.y = theJ;
    theEulerAngle.z = theH;
    theEulerAngle.order = theOrder;
    return theEulerAngle;
}

//==============================================================================
/**
 *  Construct quaternion from Euler angles (in radians).
 *  @param theEulerAngle        incoming angle( radians )
 *  @return the Quaternion
 */
Quat QSSGEulerAngleConverter::eulerToQuat(EulerAngles theEulerAngle)
{
    Quat theQuaternion;
    double a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;
    Q_UNUSED(h)

    extractEulerOrder(theEulerAngle.order, i, j, k, h, n, s, f);
    if (f == EulFrmR) {
        float t = theEulerAngle.x;
        theEulerAngle.x = theEulerAngle.z;
        theEulerAngle.z = t;
    }

    if (n == EulParOdd)
        theEulerAngle.y = -theEulerAngle.y;

    ti = theEulerAngle.x * 0.5;
    tj = theEulerAngle.y * 0.5;
    th = theEulerAngle.z * 0.5;

    ci = cos(ti);
    cj = cos(tj);
    ch = cos(th);

    si = sin(ti);
    sj = sin(tj);
    sh = sin(th);

    cc = ci * ch;
    cs = ci * sh;
    sc = si * ch;
    ss = si * sh;

    if (s == EulRepYes) {
        a[i] = cj * (cs + sc); /* Could speed up with */
        a[j] = sj * (cc + ss); /* trig identities. */
        a[k] = sj * (cs - sc);
        theQuaternion.w = (float)(cj * (cc - ss));
    } else {
        a[i] = cj * sc - sj * cs;
        a[j] = cj * ss + sj * cc;
        a[k] = cj * cs - sj * sc;
        theQuaternion.w = (float)(cj * cc + sj * ss);
    }
    if (n == EulParOdd)
        a[j] = -a[j];

    theQuaternion.x = (float)a[X];
    theQuaternion.y = (float)a[Y];
    theQuaternion.z = (float)a[Z];
    return theQuaternion;
}

//==============================================================================
/**
 *  Construct matrix from Euler angles (in radians).
 *  @param theEulerAngle        incoming angle
 *  @param theMatrix            outgoing matrix
 */
void QSSGEulerAngleConverter::eulerToHMatrix(EulerAngles theEulerAngle, HMatrix theMatrix)
{
    double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;
    Q_UNUSED(h)
    extractEulerOrder(theEulerAngle.order, i, j, k, h, n, s, f);

    if (f == EulFrmR) {
        float t = theEulerAngle.x;
        theEulerAngle.x = theEulerAngle.z;
        theEulerAngle.z = t;
    }

    if (n == EulParOdd) {
        theEulerAngle.x = -theEulerAngle.x;
        theEulerAngle.y = -theEulerAngle.y;
        theEulerAngle.z = -theEulerAngle.z;
    }

    ti = theEulerAngle.x;
    tj = theEulerAngle.y;
    th = theEulerAngle.z;

    ci = cos(ti);
    cj = cos(tj);
    ch = cos(th);

    si = sin(ti);
    sj = sin(tj);
    sh = sin(th);

    cc = ci * ch;
    cs = ci * sh;
    sc = si * ch;
    ss = si * sh;

    if (s == EulRepYes) {
        theMatrix[i][i] = (float)cj;
        theMatrix[i][j] = (float)(sj * si);
        theMatrix[i][k] = (float)(sj * ci);
        theMatrix[j][i] = (float)(sj * sh);
        theMatrix[j][j] = (float)(-cj * ss + cc);
        theMatrix[j][k] = (float)(-cj * cs - sc);
        theMatrix[k][i] = (float)(-sj * ch);
        theMatrix[k][j] = (float)(cj * sc + cs);
        theMatrix[k][k] = (float)(cj * cc - ss);
    } else {
        theMatrix[i][i] = (float)(cj * ch);
        theMatrix[i][j] = (float)(sj * sc - cs);
        theMatrix[i][k] = (float)(sj * cc + ss);
        theMatrix[j][i] = (float)(cj * sh);
        theMatrix[j][j] = (float)(sj * ss + cc);
        theMatrix[j][k] = (float)(sj * cs - sc);
        theMatrix[k][i] = (float)(-sj);
        theMatrix[k][j] = (float)(cj * si);
        theMatrix[k][k] = (float)(cj * ci);
    }

    theMatrix[W][X] = 0.0;
    theMatrix[W][Y] = 0.0;
    theMatrix[W][Z] = 0.0;
    theMatrix[X][W] = 0.0;
    theMatrix[Y][W] = 0.0;
    theMatrix[Z][W] = 0.0;
    theMatrix[W][W] = 1.0;
}

//==============================================================================
/**
 *  Convert matrix to Euler angles (in radians).
 *  @param theMatrix            incoming matrix
 *  @param theOrder             use getEulerOrder() to generate this enum
 *  @return a set of angles in radians!!!!
 */
EulerAngles QSSGEulerAngleConverter::eulerFromHMatrix(HMatrix theMatrix, EulerOrder theOrder)
{
    EulerAngles theEulerAngle;
    int i, j, k, h, n, s, f;
    Q_UNUSED(h)

    extractEulerOrder(theOrder, i, j, k, h, n, s, f);
    if (s == EulRepYes) {
        double sy = sqrt(theMatrix[i][j] * theMatrix[i][j] + theMatrix[i][k] * theMatrix[i][k]);
        if (sy > 16 * FLT_EPSILON) {
            theEulerAngle.x = (float)(atan2((double)theMatrix[i][j], (double)theMatrix[i][k]));
            theEulerAngle.y = (float)(atan2((double)sy, (double)theMatrix[i][i]));
            theEulerAngle.z = (float)(atan2((double)theMatrix[j][i], -(double)theMatrix[k][i]));
        } else {
            theEulerAngle.x = (float)(atan2(-(double)theMatrix[j][k], (double)theMatrix[j][j]));
            theEulerAngle.y = (float)(atan2((double)sy, (double)theMatrix[i][i]));
            theEulerAngle.z = 0;
        }
    } else {
        double cy = sqrt(theMatrix[i][i] * theMatrix[i][i] + theMatrix[j][i] * theMatrix[j][i]);
        if (cy > 16 * FLT_EPSILON) {
            theEulerAngle.x = (float)(atan2((double)theMatrix[k][j], (double)theMatrix[k][k]));
            theEulerAngle.y = (float)(atan2(-(double)theMatrix[k][i], (double)cy));
            theEulerAngle.z = (float)(atan2((double)theMatrix[j][i], (double)theMatrix[i][i]));
        } else {
            theEulerAngle.x = (float)(atan2(-(double)theMatrix[j][k], (double)theMatrix[j][j]));
            theEulerAngle.y = (float)(atan2(-(double)theMatrix[k][i], (double)cy));
            theEulerAngle.z = 0;
        }
    }

    if (n == EulParOdd) {
        theEulerAngle.x = -theEulerAngle.x;
        theEulerAngle.y = -theEulerAngle.y;
        theEulerAngle.z = -theEulerAngle.z;
    }

    if (f == EulFrmR) {
        float t = theEulerAngle.x;
        theEulerAngle.x = theEulerAngle.z;
        theEulerAngle.z = t;
    }
    theEulerAngle.order = theOrder;
    return theEulerAngle;
}

//==============================================================================
/**
 *  Convert quaternion to Euler angles (in radians).
 *  @param theQuaternion        incoming quaternion
 *  @param theOrder             use getEulerOrder() to generate this enum
 *  @return the generated angles ( radians )
 */
EulerAngles QSSGEulerAngleConverter::eulerFromQuat(Quat theQuaternion, EulerOrder theOrder)
{
    HMatrix theMatrix;
    double Nq = theQuaternion.x * theQuaternion.x + theQuaternion.y * theQuaternion.y
            + theQuaternion.z * theQuaternion.z + theQuaternion.w * theQuaternion.w;
    double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
    double xs = theQuaternion.x * s;
    double ys = theQuaternion.y * s;
    double zs = theQuaternion.z * s;
    double wx = theQuaternion.w * xs;
    double wy = theQuaternion.w * ys;
    double wz = theQuaternion.w * zs;
    double xx = theQuaternion.x * xs;
    double xy = theQuaternion.x * ys;
    double xz = theQuaternion.x * zs;
    double yy = theQuaternion.y * ys;
    double yz = theQuaternion.y * zs;
    double zz = theQuaternion.z * zs;

    theMatrix[X][X] = (float)(1.0 - (yy + zz));
    theMatrix[X][Y] = (float)(xy - wz);
    theMatrix[X][Z] = (float)(xz + wy);
    theMatrix[Y][X] = (float)(xy + wz);
    theMatrix[Y][Y] = (float)(1.0 - (xx + zz));
    theMatrix[Y][Z] = (float)(yz - wx);
    theMatrix[Z][X] = (float)(xz - wy);
    theMatrix[Z][Y] = (float)(yz + wx);
    theMatrix[Z][Z] = (float)(1.0 - (xx + yy));
    theMatrix[W][X] = 0.0;
    theMatrix[W][Y] = 0.0;
    theMatrix[W][Z] = 0.0;
    theMatrix[X][W] = 0.0;
    theMatrix[Y][W] = 0.0;
    theMatrix[Z][W] = 0.0;
    theMatrix[W][W] = 1.0;

    return eulerFromHMatrix(theMatrix, theOrder);
}

//==============================================================================

// Create some mapping of euler angles to their axis mapping.
#define ITERATE_POSSIBLE_EULER_ANGLES                                                                                  \
    HANDLE_EULER_ANGLE(EulerOrder::XYZs, X, Y, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XYXs, X, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XZYs, X, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XZXs, X, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YZXs, Y, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YZYs, Y, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YXZs, Y, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YXYs, Y, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZXYs, Z, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZXZs, Z, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZYXs, Z, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZYZs, Z, Y, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZYXr, Z, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XYXr, X, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YZXr, Y, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XZXr, X, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XZYr, X, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YZYr, Y, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZXYr, Z, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YXYr, Y, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::YXZr, Y, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZXZr, Z, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::XYZr, X, Y, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulerOrder::ZYZr, Z, Y, Z)

EulerAngles QSSGEulerAngleConverter::calculateEulerAngles(const QVector3D &rotation, EulerOrder order)
{
    EulerAngles retval;
    retval.order = order;
    int X = 0;
    int Y = 1;
    int Z = 2;

    switch (order) {
#define HANDLE_EULER_ANGLE(order, xIdx, yIdx, zIdx)                                                                  \
    case order:                                                                                                      \
        retval.x = -rotation[xIdx];                                                                                  \
        retval.y = -rotation[yIdx];                                                                                  \
        retval.z = -rotation[zIdx];                                                                                  \
        break;
        ITERATE_POSSIBLE_EULER_ANGLES
#undef HANDLE_EULER_ANGLE
    default:
        Q_ASSERT(false);
        retval.x = rotation[X];
        retval.y = rotation[Y];
        retval.z = rotation[Z];
        break;
    }
    return retval;
}

QVector3D QSSGEulerAngleConverter::calculateRotationVector(const EulerAngles &angles)
{
    QVector3D retval(0, 0, 0);
    int X = 0;
    int Y = 1;
    int Z = 2;
    switch (angles.order) {
#define HANDLE_EULER_ANGLE(order, xIdx, yIdx, zIdx)                                                                  \
    case order:                                                                                                      \
        retval[xIdx] = -angles.x;                                                                                    \
        retval[yIdx] = -angles.y;                                                                                    \
        retval[zIdx] = -angles.z;                                                                                    \
        break;
        ITERATE_POSSIBLE_EULER_ANGLES
#undef HANDLE_EULER_ANGLE
    default:
        Q_ASSERT(false);
        retval.setX(angles.x);
        retval.setY(angles.y);
        retval.setZ(angles.z);
        break;
    }

    return retval;
}

QMatrix4x4 QSSGEulerAngleConverter::createRotationMatrix(const QVector3D &rotationAsRadians, EulerOrder order)
{
    QMatrix4x4 matrix;
    const EulerAngles theAngles = QSSGEulerAngleConverter::calculateEulerAngles(rotationAsRadians, order);
    QSSGEulerAngleConverter theConverter;
    theConverter.eulerToHMatrix(theAngles, *reinterpret_cast<HMatrix *>(&matrix));
    return matrix;
}

QVector3D QSSGEulerAngleConverter::calculateRotationVector(const QMatrix3x3 &rotationMatrix,
                                                           bool matrixIsLeftHanded,
                                                           EulerOrder order)
{
    QMatrix4x4 theConvertMatrix = { rotationMatrix(0, 0),
                                    rotationMatrix(0, 1),
                                    rotationMatrix(0, 2),
                                    0.0f,
                                    rotationMatrix(1, 0),
                                    rotationMatrix(1, 1),
                                    rotationMatrix(1, 2),
                                    0.0f,
                                    rotationMatrix(2, 0),
                                    rotationMatrix(2, 1),
                                    rotationMatrix(2, 2),
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    1.0f };

    if (matrixIsLeftHanded)
        mat44::flip(theConvertMatrix);

    QSSGEulerAngleConverter theConverter;
    HMatrix *theHMatrix = reinterpret_cast<HMatrix *>(theConvertMatrix.data());
    EulerAngles theAngles = theConverter.eulerFromHMatrix(*theHMatrix, order);
    return calculateRotationVector(theAngles);
}

//==============================================================================
/**
 *	Dump the Order information
 */
const char *QSSGEulerAngleConverter::dumpOrderInfo()
{
    long theCount = 0;
    long theOrder[24];
    char theOrderStr[24][16];

    ::strcpy(theOrderStr[theCount++], "EulerOrder::XYZs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XYXs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XZYs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XZXs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YZXs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YZYs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YXZs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YXYs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZXYs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZXZs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZYXs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZYZs");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZYXr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XYXr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YZXr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XZXr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XZYr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YZYr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZXYr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YXYr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::YXZr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZXZr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::XYZr");
    ::strcpy(theOrderStr[theCount++], "EulerOrder::ZYZr");

    theCount = 0;
    theOrder[theCount++] = EulerOrder::XYZs;
    theOrder[theCount++] = EulerOrder::XYXs;
    theOrder[theCount++] = EulerOrder::XZYs;
    theOrder[theCount++] = EulerOrder::XZXs;
    theOrder[theCount++] = EulerOrder::YZXs;
    theOrder[theCount++] = EulerOrder::YZYs;
    theOrder[theCount++] = EulerOrder::YXZs;
    theOrder[theCount++] = EulerOrder::YXYs;
    theOrder[theCount++] = EulerOrder::ZXYs;
    theOrder[theCount++] = EulerOrder::ZXZs;
    theOrder[theCount++] = EulerOrder::ZYXs;
    theOrder[theCount++] = EulerOrder::ZYZs;

    theOrder[theCount++] = EulerOrder::ZYXr;
    theOrder[theCount++] = EulerOrder::XYXr;
    theOrder[theCount++] = EulerOrder::YZXr;
    theOrder[theCount++] = EulerOrder::XZXr;
    theOrder[theCount++] = EulerOrder::XZYr;
    theOrder[theCount++] = EulerOrder::YZYr;
    theOrder[theCount++] = EulerOrder::ZXYr;
    theOrder[theCount++] = EulerOrder::YXYr;
    theOrder[theCount++] = EulerOrder::YXZr;
    theOrder[theCount++] = EulerOrder::ZXZr;
    theOrder[theCount++] = EulerOrder::XYZr;
    theOrder[theCount++] = EulerOrder::ZYZr;

    char theSubBuf[256];
    m_orderInfoBuffer[0] = '\0';
    for (long theIndex = 0; theIndex < 24; ++theIndex) {
        ::sprintf(theSubBuf, " %16s - %ld\n ", theOrderStr[theIndex], theOrder[theIndex]);
        ::strcat(m_orderInfoBuffer, theSubBuf);
    }

    return m_orderInfoBuffer;
}
QT_END_NAMESPACE
