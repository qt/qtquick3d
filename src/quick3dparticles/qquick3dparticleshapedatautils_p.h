/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQUICK3DPARTICLESHAPEDATAUTILS_P_H
#define QQUICK3DPARTICLESHAPEDATAUTILS_P_H

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

#include <QtQuick3DParticles/qtquick3dparticlesglobal.h>
#include <QtCore/QCborStreamReader>
#include <QtCore/QCborStreamWriter>
#include <QtCore/QVariant>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleShapeDataUtils
{
public:
    static QVariant readValue(QCborStreamReader &reader, QMetaType::Type type);
    static QString readString(QCborStreamReader &reader);
    static double readReal(QCborStreamReader &reader);
    static int readShapeHeader(QCborStreamReader &reader);
    static void writeShapeHeader(QCborStreamWriter &writer, int version = 1);
    static void writeValue(QCborStreamWriter &writer, const QVariant &value);
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLESHAPEDATAUTILS_P_H
