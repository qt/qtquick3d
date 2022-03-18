/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef HEIGHTFIELDGEOMETRY_H
#define HEIGHTFIELDGEOMETRY_H

#include <QtQuick3D/private/qquick3dgeometry_p.h>

// Workaround for QTBUG-94099, ensures qml_register_types...() is exported
#include "qtquick3dhelpersglobal_p.h"

QT_BEGIN_NAMESPACE

class HeightFieldGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(QVector3D heightFieldScale READ heightFieldScale WRITE setHeightFieldScale NOTIFY heightFieldScaleChanged)
    Q_PROPERTY(QUrl heightMap READ heightMap WRITE setHeightMap NOTIFY heightMapChanged)
    QML_NAMED_ELEMENT(HeightFieldGeometry)
public:
    HeightFieldGeometry();

    const QVector3D &heightFieldScale() const;
    void setHeightFieldScale(const QVector3D &newHeightFieldScale);

    const QUrl &heightMap() const;
    void setHeightMap(const QUrl &newHeightMap);

signals:
    void heightFieldScaleChanged();
    void heightMapChanged();

private:
    void updateData();
    QVector3D m_heightFieldScale = { 1, 1, 1 };
    QUrl m_heightMapSource;
};

QT_END_NAMESPACE

#endif
