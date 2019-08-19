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

#ifndef QSSGRENDEREULERANGLES_H
#define QSSGRENDEREULERANGLES_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <qmath.h>
#include <QVector3D>
#include <QMatrix3x3>
#include <QMatrix4x4>

QT_BEGIN_NAMESPACE
//==============================================================================
//	Description
//==============================================================================
// QuatTypes.h - Basic type declarations
// by Ken Shoemake, shoemake@graphics.cis.upenn.edu
// in "Graphics Gems IV", Academic Press, 1994
typedef struct
{
    float x, y, z, w;
} Quat; /* Quaternion */
typedef float HMatrix[4][4]; /* Right-handed, for column vectors */
enum QuatPart { X, Y, Z, W };
typedef Quat EulerAngles; /* (x,y,z)=ang 1,2,3, w=order code  */

#define EulFrmS 0
#define EulFrmR 1
#define EulFrm(ord) ((unsigned)(ord)&1)
#define EulRepNo 0
#define EulRepYes 1
#define EulRep(ord) (((unsigned)(ord) >> 1) & 1)
#define EulParEven 0
#define EulParOdd 1
#define EulPar(ord) (((unsigned)(ord) >> 2) & 1)
#define EulSafe "\000\001\002\000"
#define EulNext "\001\002\000\001"
#define EulAxI(ord) ((int)(EulSafe[(((unsigned)(ord) >> 3) & 3)]))
#define EulAxJ(ord) ((int)(EulNext[EulAxI(ord) + (EulPar(ord) == EulParOdd)]))
#define EulAxK(ord) ((int)(EulNext[EulAxI(ord) + (EulPar(ord) != EulParOdd)]))
#define EulAxH(ord) ((EulRep(ord) == EulRepNo) ? EulAxK(ord) : EulAxI(ord))

// EulGetOrd unpacks all useful information about order simultaneously.
#define EulGetOrd(ord, i, j, k, h, n, s, f)                                                                            \
    {                                                                                                                  \
        unsigned o = ord;                                                                                              \
        f = o & 1;                                                                                                     \
        o = o >> 1;                                                                                                    \
        s = o & 1;                                                                                                     \
        o = o >> 1;                                                                                                    \
        n = o & 1;                                                                                                     \
        o = o >> 1;                                                                                                    \
        i = EulSafe[o & 3];                                                                                            \
        j = EulNext[i + n];                                                                                            \
        k = EulNext[i + 1 - n];                                                                                        \
        h = s ? k : i;                                                                                                 \
    }

// EulOrd creates an order value between 0 and 23 from 4-tuple choices.
#define EulOrd(i, p, r, f) (((((((i) << 1) + (p)) << 1) + (r)) << 1) + (f))

// Static axes
// X = 0, Y = 1, Z = 2 ref QuatPart
#define EulOrdXYZs EulOrd(0, EulParEven, EulRepNo, EulFrmS)
#define EulOrdXYXs EulOrd(0, EulParEven, EulRepYes, EulFrmS)
#define EulOrdXZYs EulOrd(0, EulParOdd, EulRepNo, EulFrmS)
#define EulOrdXZXs EulOrd(0, EulParOdd, EulRepYes, EulFrmS)
#define EulOrdYZXs EulOrd(1, EulParEven, EulRepNo, EulFrmS)
#define EulOrdYZYs EulOrd(1, EulParEven, EulRepYes, EulFrmS)
#define EulOrdYXZs EulOrd(1, EulParOdd, EulRepNo, EulFrmS)
#define EulOrdYXYs EulOrd(1, EulParOdd, EulRepYes, EulFrmS)
#define EulOrdZXYs EulOrd(2, EulParEven, EulRepNo, EulFrmS)
#define EulOrdZXZs EulOrd(2, EulParEven, EulRepYes, EulFrmS)
#define EulOrdZYXs EulOrd(2, EulParOdd, EulRepNo, EulFrmS)
#define EulOrdZYZs EulOrd(2, EulParOdd, EulRepYes, EulFrmS)

// Rotating axes
#define EulOrdZYXr EulOrd(0, EulParEven, EulRepNo, EulFrmR)
#define EulOrdXYXr EulOrd(0, EulParEven, EulRepYes, EulFrmR)
#define EulOrdYZXr EulOrd(0, EulParOdd, EulRepNo, EulFrmR)
#define EulOrdXZXr EulOrd(0, EulParOdd, EulRepYes, EulFrmR)
#define EulOrdXZYr EulOrd(1, EulParEven, EulRepNo, EulFrmR)
#define EulOrdYZYr EulOrd(1, EulParEven, EulRepYes, EulFrmR)
#define EulOrdZXYr EulOrd(1, EulParOdd, EulRepNo, EulFrmR)
#define EulOrdYXYr EulOrd(1, EulParOdd, EulRepYes, EulFrmR)
#define EulOrdYXZr EulOrd(2, EulParEven, EulRepNo, EulFrmR)
#define EulOrdZXZr EulOrd(2, EulParEven, EulRepYes, EulFrmR)
#define EulOrdXYZr EulOrd(2, EulParOdd, EulRepNo, EulFrmR)
#define EulOrdZYZr EulOrd(2, EulParOdd, EulRepYes, EulFrmR)

#ifndef M_PI
#define M_PI 3.1415926535898
#endif

#define TODEG(x) x = (float)(x * 180 / M_PI);
#define TORAD(x) x = (float)(x / 180 * M_PI);

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGEulerAngleConverter
{
private:
    char m_orderInfoBuffer[1024];

public:
    QSSGEulerAngleConverter();
    ~QSSGEulerAngleConverter();

public:
    EulerAngles euler(float ai, float aj, float ah, int order);
    Quat eulerToQuat(EulerAngles ea);
    void eulerToHMatrix(EulerAngles ea, HMatrix M);
    EulerAngles eulerFromHMatrix(HMatrix M, int order);
    EulerAngles eulerFromQuat(Quat q, int order);

    static EulerAngles calculateEulerAngles(const QVector3D &rotation, quint32 order);
    static QVector3D calculateRotationVector(const EulerAngles &angles);
    static QVector3D calculateRotationVector(const QMatrix3x3 &rotationMatrix, bool matrixIsLeftHanded, quint32 order);
    static QMatrix4x4 createRotationMatrix(const QVector3D &rotationAsRadians, quint32 order);

    // Debug Stuff
    const char *dumpOrderInfo();
};
QT_END_NAMESPACE

#endif // QSSGRENDEREULERANGLES_H
