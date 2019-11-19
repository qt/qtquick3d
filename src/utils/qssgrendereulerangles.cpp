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

#include "qssgrendereulerangles_p.h"
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

namespace {
// extractEulerOrder unpacks all useful information about order simultaneously.
inline void extractEulerOrder(EulerOrder eulerOrder, int &i, int &j, int &k, int &h, int &n, int &s, int &f)
{
    uint order = eulerOrder;
    f = order & 1;
    order = order >> 1;
    s = order & 1;
    order = order >> 1;
    n = order & 1;
    order = order >> 1;
    i = EulSafe[order & 3];
    j = EulNext[i + n];
    k = EulNext[i + 1 - n];
    h = s ? k : i;
}

enum QuatPart { X, Y, Z, W };
}

/**
 *	Constructor
 */
QSSGEulerAngleConverter::QSSGEulerAngleConverter() = default;

/**
 *	Destructor
 */
QSSGEulerAngleConverter::~QSSGEulerAngleConverter() = default;

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

/**
 *  Construct quaternion from Euler angles (in radians).
 *  @param theEulerAngle        incoming angle( radians )
 *  @return the Quaternion
 */
QSSGEulerAngleConverter::Quat QSSGEulerAngleConverter::eulerToQuat(EulerAngles theEulerAngle)
{
    Quat theQuaternion;
    double a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;

    extractEulerOrder(theEulerAngle.order, i, j, k, h, n, s, f);

    if (f == EulFrmR)
        std::swap(theEulerAngle.x, theEulerAngle.z);

    if (n == EulParOdd)
        theEulerAngle.y *= -1;

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
        theQuaternion.w = float(cj * (cc - ss));
    } else {
        a[i] = cj * sc - sj * cs;
        a[j] = cj * ss + sj * cc;
        a[k] = cj * cs - sj * sc;
        theQuaternion.w = float(cj * cc + sj * ss);
    }
    if (n == EulParOdd)
        a[j] = -a[j];

    theQuaternion.x = float(a[X]);
    theQuaternion.y = float(a[Y]);
    theQuaternion.z = float(a[Z]);
    return theQuaternion;
}

/**
 *  Construct matrix from Euler angles (in radians).
 *  @param theEulerAngle        incoming angle
 *  @param theMatrix            outgoing matrix
 */
void QSSGEulerAngleConverter::eulerToHMatrix(EulerAngles theEulerAngle, HMatrix theMatrix)
{
    double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i, j, k, h, n, s, f;

    extractEulerOrder(theEulerAngle.order, i, j, k, h, n, s, f);

    if (f == EulFrmR)
        std::swap(theEulerAngle.x, theEulerAngle.z);

    if (n == EulParOdd) {
        theEulerAngle.x *= -1;
        theEulerAngle.y *= -1;
        theEulerAngle.z *= -1;
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
        theMatrix[i][i] = float(cj);
        theMatrix[i][j] = float(sj * si);
        theMatrix[i][k] = float(sj * ci);
        theMatrix[j][i] = float(sj * sh);
        theMatrix[j][j] = float(-cj * ss + cc);
        theMatrix[j][k] = float(-cj * cs - sc);
        theMatrix[k][i] = float(-sj * ch);
        theMatrix[k][j] = float(cj * sc + cs);
        theMatrix[k][k] = float(cj * cc - ss);
    } else {
        theMatrix[i][i] = float(cj * ch);
        theMatrix[i][j] = float(sj * sc - cs);
        theMatrix[i][k] = float(sj * cc + ss);
        theMatrix[j][i] = float(cj * sh);
        theMatrix[j][j] = float(sj * ss + cc);
        theMatrix[j][k] = float(sj * cs - sc);
        theMatrix[k][i] = float(-sj);
        theMatrix[k][j] = float(cj * si);
        theMatrix[k][k] = float(cj * ci);
    }

    theMatrix[W][X] = 0.0;
    theMatrix[W][Y] = 0.0;
    theMatrix[W][Z] = 0.0;
    theMatrix[X][W] = 0.0;
    theMatrix[Y][W] = 0.0;
    theMatrix[Z][W] = 0.0;
    theMatrix[W][W] = 1.0;
}

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

    extractEulerOrder(theOrder, i, j, k, h, n, s, f);

    double ij = double(theMatrix[i][j]), ik = double(theMatrix[i][k]);
    double ii = double(theMatrix[i][i]), ji = double(theMatrix[j][i]);
    double kk = double(theMatrix[k][k]), jj = double(theMatrix[j][j]);
    double ki = double(theMatrix[k][i]), kj = double(theMatrix[k][j]);
    double jk = double(theMatrix[j][k]);
    if (s == EulRepYes) {
        double sy = sqrt(ij*ij + ik*ik);
        theEulerAngle.y = float(atan2(sy, ii));
        if (!qFuzzyIsNull(float(sy))) {
            theEulerAngle.x = float(atan2(ij, ik));
            theEulerAngle.z = float(atan2(ji, -ki));
        } else {
            theEulerAngle.x = float(atan2(-jk, jj));
            theEulerAngle.z = 0;
        }
    } else {
        double cy = sqrt(ii*ii + ji*ji);
        theEulerAngle.y = float(atan2(-ki, cy));
        if (!qFuzzyIsNull(float(cy))) {
            theEulerAngle.x = float(atan2(kj, kk));
            theEulerAngle.z = float(atan2(ji, ii));
        } else {
            theEulerAngle.x = float(atan2(-jk, jj));
            theEulerAngle.z = 0;
        }
    }

    if (n == EulParOdd) {
        theEulerAngle.x *= -1;
        theEulerAngle.y *= -1;
        theEulerAngle.z *= -1;
    }

    if (f == EulFrmR)
        std::swap(theEulerAngle.x, theEulerAngle.z);

    theEulerAngle.order = theOrder;
    return theEulerAngle;
}

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

    theMatrix[X][X] = float(1.0 - (yy + zz));
    theMatrix[X][Y] = float(xy - wz);
    theMatrix[X][Z] = float(xz + wy);
    theMatrix[Y][X] = float(xy + wz);
    theMatrix[Y][Y] = float(1.0 - (xx + zz));
    theMatrix[Y][Z] = float(yz - wx);
    theMatrix[Z][X] = float(xz - wy);
    theMatrix[Z][Y] = float(yz + wx);
    theMatrix[Z][Z] = float(1.0 - (xx + yy));
    theMatrix[W][X] = 0.0;
    theMatrix[W][Y] = 0.0;
    theMatrix[W][Z] = 0.0;
    theMatrix[X][W] = 0.0;
    theMatrix[Y][W] = 0.0;
    theMatrix[Z][W] = 0.0;
    theMatrix[W][W] = 1.0;

    return eulerFromHMatrix(theMatrix, theOrder);
}

EulerAngles QSSGEulerAngleConverter::calculateEulerAngles(const QVector3D &rotation, EulerOrder order)
{
    EulerAngles retval;
    retval.order = order;
    const int X = 0;
    const int Y = 1;
    const int Z = 2;

    switch (order) {
    case EulerOrder::XYZs:
        retval.x = -rotation[X];
        retval.y = -rotation[Y];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::XYXs:
        retval.x = -rotation[X];
        retval.y = -rotation[Y];
        retval.z = -rotation[X];
        break;
    case EulerOrder::XZYs:
        retval.x = -rotation[X];
        retval.y = -rotation[Z];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::XZXs:
        retval.x = -rotation[X];
        retval.y = -rotation[Z];
        retval.z = -rotation[X];
        break;
    case EulerOrder::YZXs:
        retval.x = -rotation[Y];
        retval.y = -rotation[Z];
        retval.z = -rotation[X];
        break;
    case EulerOrder::YZYs:
        retval.x = -rotation[Y];
        retval.y = -rotation[Z];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::YXZs:
        retval.x = -rotation[Y];
        retval.y = -rotation[X];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::YXYs:
        retval.x = -rotation[Y];
        retval.y = -rotation[X];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::ZXYs:
        retval.x = -rotation[Z];
        retval.y = -rotation[X];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::ZXZs:
        retval.x = -rotation[Z];
        retval.y = -rotation[X];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::ZYXs:
        retval.x = -rotation[Z];
        retval.y = -rotation[Y];
        retval.z = -rotation[X];
        break;
    case EulerOrder::ZYZs:
        retval.x = -rotation[Z];
        retval.y = -rotation[Y];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::ZYXr:
        retval.x = -rotation[Z];
        retval.y = -rotation[Y];
        retval.z = -rotation[X];
        break;
    case EulerOrder::XYXr:
        retval.x = -rotation[X];
        retval.y = -rotation[Y];
        retval.z = -rotation[X];
        break;
    case EulerOrder::YZXr:
        retval.x = -rotation[Y];
        retval.y = -rotation[Z];
        retval.z = -rotation[X];
        break;
    case EulerOrder::XZXr:
        retval.x = -rotation[X];
        retval.y = -rotation[Z];
        retval.z = -rotation[X];
        break;
    case EulerOrder::XZYr:
        retval.x = -rotation[X];
        retval.y = -rotation[Z];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::YZYr:
        retval.x = -rotation[Y];
        retval.y = -rotation[Z];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::ZXYr:
        retval.x = -rotation[Z];
        retval.y = -rotation[X];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::YXYr:
        retval.x = -rotation[Y];
        retval.y = -rotation[X];
        retval.z = -rotation[Y];
        break;
    case EulerOrder::YXZr:
        retval.x = -rotation[Y];
        retval.y = -rotation[X];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::ZXZr:
        retval.x = -rotation[Z];
        retval.y = -rotation[X];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::XYZr:
        retval.x = -rotation[X];
        retval.y = -rotation[Y];
        retval.z = -rotation[Z];
        break;
    case EulerOrder::ZYZr:
        retval.x = -rotation[Z];
        retval.y = -rotation[Y];
        retval.z = -rotation[Z];
        break;
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
    const int X = 0;
    const int Y = 1;
    const int Z = 2;

    switch (angles.order) {
    case EulerOrder::XYZs:
        retval[X] = -angles.x;
        retval[Y] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::XYXs:
        retval[X] = -angles.x;
        retval[Y] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::XZYs:
        retval[X] = -angles.x;
        retval[Z] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::XZXs:
        retval[X] = -angles.x;
        retval[Z] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::YZXs:
        retval[Y] = -angles.x;
        retval[Z] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::YZYs:
        retval[Y] = -angles.x;
        retval[Z] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::YXZs:
        retval[Y] = -angles.x;
        retval[X] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::YXYs:
        retval[Y] = -angles.x;
        retval[X] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::ZXYs:
        retval[Z] = -angles.x;
        retval[X] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::ZXZs:
        retval[Z] = -angles.x;
        retval[X] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::ZYXs:
        retval[Z] = -angles.x;
        retval[Y] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::ZYZs:
        retval[Z] = -angles.x;
        retval[Y] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::ZYXr:
        retval[Z] = -angles.x;
        retval[Y] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::XYXr:
        retval[X] = -angles.x;
        retval[Y] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::YZXr:
        retval[Y] = -angles.x;
        retval[Z] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::XZXr:
        retval[X] = -angles.x;
        retval[Z] = -angles.y;
        retval[X] = -angles.z;
        break;
    case EulerOrder::XZYr:
        retval[X] = -angles.x;
        retval[Z] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::YZYr:
        retval[Y] = -angles.x;
        retval[Z] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::ZXYr:
        retval[Z] = -angles.x;
        retval[X] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::YXYr:
        retval[Y] = -angles.x;
        retval[X] = -angles.y;
        retval[Y] = -angles.z;
        break;
    case EulerOrder::YXZr:
        retval[Y] = -angles.x;
        retval[X] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::ZXZr:
        retval[Z] = -angles.x;
        retval[X] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::XYZr:
        retval[X] = -angles.x;
        retval[Y] = -angles.y;
        retval[Z] = -angles.z;
        break;
    case EulerOrder::ZYZr:
        retval[Z] = -angles.x;
        retval[Y] = -angles.y;
        retval[Z] = -angles.z;
        break;
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
    QSSGEulerAngleConverter::eulerToHMatrix(theAngles, *reinterpret_cast<HMatrix *>(&matrix));
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

    HMatrix *theHMatrix = reinterpret_cast<HMatrix *>(theConvertMatrix.data());
    EulerAngles theAngles = QSSGEulerAngleConverter::eulerFromHMatrix(*theHMatrix, order);
    return calculateRotationVector(theAngles);
}
QT_END_NAMESPACE
