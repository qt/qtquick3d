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
    Q_PROPERTY(QUrl heightMap READ heightMap WRITE setHeightMap NOTIFY heightMapChanged)
    Q_PROPERTY(bool smoothShading READ smoothShading WRITE setSmoothShading NOTIFY smoothShadingChanged)
    Q_PROPERTY(QVector3D extents READ extents WRITE setExtents NOTIFY extentsChanged)
    QML_NAMED_ELEMENT(HeightFieldGeometry)
public:
    HeightFieldGeometry();

    const QUrl &heightMap() const;
    void setHeightMap(const QUrl &newHeightMap);

    bool smoothShading() const;
    void setSmoothShading(bool smooth);
    const QVector3D &extents() const;
    void setExtents(const QVector3D &newExtents);

signals:
    void heightMapChanged();
    void smoothShadingChanged();
    void extentsChanged();

private:
    void updateData();
    QVector3D m_extents = { 100, 100, 100 };
    QUrl m_heightMapSource;
    bool m_smoothShading = true;
    bool m_extentsSetExplicitly = false;
};

QT_END_NAMESPACE

#endif
