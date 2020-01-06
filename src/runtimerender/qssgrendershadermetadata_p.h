/****************************************************************************
**
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

#ifndef QSSGRENDERSHADERMETADATA_P_H
#define QSSGRENDERSHADERMETADATA_P_H

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

#include <QtCore/qbytearray.h>

namespace QSSGRenderShaderMetadata {

struct Uniform
{
    enum Type
    {
        Invalid = -1,
        Boolean = 0,
        Int,
        Uint,
        Float,
        Double,
        Sampler,
        Vec2 = 0x010,
        Vec3 = 0x020,
        Vec4 = 0x040,
        Mat = 0x100, // The nxm data is encoded in 3 bits chunks at the end
    };

    enum Condition
    {
        None,
        Regular,
        Negated
    };

    int type = Type::Invalid;
    Condition condition = Condition::None;
    QByteArray name;
    QByteArray conditionName;

    static int typeFromString(const QString &str);
    static Condition conditionFromString(const QString &condition);
};

using UniformList = QVector<Uniform>;

UniformList getShaderMetaData(const QByteArray &data);

} // namespace

#endif // QSSGRENDERSHADERMETADATA_H
