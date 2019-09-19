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
 *	Constructs a Euler angle & holds it in a EulerAngles struct
 *	@param theI			x rotation ( radians )
 *	@param theJ			y rotation ( radians )
 *	@param theH			z rotation ( radians )
 *	@param theOrder		the order this angle is in namely XYZ( static ), etc.
 *						use the EulOrd**** macros to generate values
 *						0 to 23 is valid
 *	@return the euler angle
 */
EulerAngles QSSGEulerAngleConverter::euler(float theI, float theJ, float theH, int theOrder)
{
    EulerAngles theEulerAngle;
    theEulerAngle.x = theI;
    theEulerAngle.y = theJ;
    theEulerAngle.z = theH;
    theEulerAngle.w = (float)theOrder;
    return theEulerAngle;
}

//==============================================================================
/**
 *	Construct quaternion from Euler angles (in radians).
 *	@param theEulerAngle		incoming angle( radians )
 *	@return the Quaternion
 */
Quat QSSGEulerAngleConverter::eulerToQuat(EulerAngles theEulerAngle)
{
    Quat theQuaternion;
    double a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;
    Q_UNUSED(h)

    EulGetOrd((unsigned int)theEulerAngle.w, i, j, k, h, n, s, f);
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
 *	Construct matrix from Euler angles (in radians).
 *	@param theEulerAngle		incoming angle
 *	@param theMatrix			outgoing matrix
 */
void QSSGEulerAngleConverter::eulerToHMatrix(EulerAngles theEulerAngle, HMatrix theMatrix)
{
    double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;
    Q_UNUSED(h)
    EulGetOrd((unsigned int)theEulerAngle.w, i, j, k, h, n, s, f);

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
 *	Convert matrix to Euler angles (in radians).
 *	@param theMatrix			incoming matrix
 *	@param theOrder				0-23, use EulOrd**** to generate this value
 *	@return a set of angles in radians!!!!
 */
EulerAngles QSSGEulerAngleConverter::eulerFromHMatrix(HMatrix theMatrix, int theOrder)
{
    EulerAngles theEulerAngle;
    int i, j, k, h, n, s, f;
    Q_UNUSED(h)

    EulGetOrd(theOrder, i, j, k, h, n, s, f);
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
    theEulerAngle.w = (float)theOrder;
    return theEulerAngle;
}

//==============================================================================
/**
 *	Convert quaternion to Euler angles (in radians).
 *	@param theQuaternion		incoming quaternion
 *	@param theOrder				0-23, use EulOrd**** to generate this value
 *	@return the generated angles ( radians )
 */
EulerAngles QSSGEulerAngleConverter::eulerFromQuat(Quat theQuaternion, int theOrder)
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
    HANDLE_EULER_ANGLE(EulOrdXYZs, X, Y, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXYXs, X, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXZYs, X, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXZXs, X, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYZXs, Y, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYZYs, Y, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYXZs, Y, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYXYs, Y, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZXYs, Z, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZXZs, Z, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZYXs, Z, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZYZs, Z, Y, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZYXr, Z, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXYXr, X, Y, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYZXr, Y, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXZXr, X, Z, X)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXZYr, X, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYZYr, Y, Z, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZXYr, Z, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYXYr, Y, X, Y)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdYXZr, Y, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZXZr, Z, X, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdXYZr, X, Y, Z)                                                                            \
    HANDLE_EULER_ANGLE(EulOrdZYZr, Z, Y, Z)

EulerAngles QSSGEulerAngleConverter::calculateEulerAngles(const QVector3D &rotation, quint32 order)
{
    EulerAngles retval;
    retval.w = float(order);
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
    switch (int(angles.w)) {
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

QMatrix4x4 QSSGEulerAngleConverter::createRotationMatrix(const QVector3D &rotationAsRadians, quint32 order)
{
    QMatrix4x4 matrix;
    const EulerAngles theAngles = QSSGEulerAngleConverter::calculateEulerAngles(rotationAsRadians, order);
    QSSGEulerAngleConverter theConverter;
    theConverter.eulerToHMatrix(theAngles, *reinterpret_cast<HMatrix *>(&matrix));
    return matrix;
}

QVector3D QSSGEulerAngleConverter::calculateRotationVector(const QMatrix3x3 &rotationMatrix, bool matrixIsLeftHanded, quint32 order)
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
    EulerAngles theAngles = theConverter.eulerFromHMatrix(*theHMatrix, int(order));
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

    ::strcpy(theOrderStr[theCount++], "EulOrdXYZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdXYXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXYs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYXs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYZs");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXYXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZXr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXZYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYZYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXYr");
    ::strcpy(theOrderStr[theCount++], "EulOrdYXZr");
    ::strcpy(theOrderStr[theCount++], "EulOrdZXZr");
    ::strcpy(theOrderStr[theCount++], "EulOrdXYZr");
    ::strcpy(theOrderStr[theCount++], "EulOrdZYZr");

    theCount = 0;
    theOrder[theCount++] = EulOrdXYZs;
    theOrder[theCount++] = EulOrdXYXs;
    theOrder[theCount++] = EulOrdXZYs;
    theOrder[theCount++] = EulOrdXZXs;
    theOrder[theCount++] = EulOrdYZXs;
    theOrder[theCount++] = EulOrdYZYs;
    theOrder[theCount++] = EulOrdYXZs;
    theOrder[theCount++] = EulOrdYXYs;
    theOrder[theCount++] = EulOrdZXYs;
    theOrder[theCount++] = EulOrdZXZs;
    theOrder[theCount++] = EulOrdZYXs;
    theOrder[theCount++] = EulOrdZYZs;

    theOrder[theCount++] = EulOrdZYXr;
    theOrder[theCount++] = EulOrdXYXr;
    theOrder[theCount++] = EulOrdYZXr;
    theOrder[theCount++] = EulOrdXZXr;
    theOrder[theCount++] = EulOrdXZYr;
    theOrder[theCount++] = EulOrdYZYr;
    theOrder[theCount++] = EulOrdZXYr;
    theOrder[theCount++] = EulOrdYXYr;
    theOrder[theCount++] = EulOrdYXZr;
    theOrder[theCount++] = EulOrdZXZr;
    theOrder[theCount++] = EulOrdXYZr;
    theOrder[theCount++] = EulOrdZYZr;

    char theSubBuf[256];
    m_orderInfoBuffer[0] = '\0';
    for (long theIndex = 0; theIndex < 24; ++theIndex) {
        ::sprintf(theSubBuf, " %16s - %ld\n ", theOrderStr[theIndex], theOrder[theIndex]);
        ::strcat(m_orderInfoBuffer, theSubBuf);
    }

    return m_orderInfoBuffer;
}
QT_END_NAMESPACE
