// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged REVISION(6, 5))
    Q_PROPERTY(bool smoothShading READ smoothShading WRITE setSmoothShading NOTIFY smoothShadingChanged)
    Q_PROPERTY(QVector3D extents READ extents WRITE setExtents NOTIFY extentsChanged)
    QML_NAMED_ELEMENT(HeightFieldGeometry)
public:
    HeightFieldGeometry();

    const QUrl &source() const;
    void setSource(const QUrl &newSource);

    bool smoothShading() const;
    void setSmoothShading(bool smooth);
    const QVector3D &extents() const;
    void setExtents(const QVector3D &newExtents);

signals:
    void sourceChanged();
    void smoothShadingChanged();
    void extentsChanged();

private:
    void updateData();
    QVector3D m_extents = { 100, 100, 100 };
    QUrl m_heightMapSource;
    bool m_smoothShading = true;
    bool m_extentsSetExplicitly = false;

#if QT_DEPRECATED_SINCE(6, 5)
    Q_PROPERTY(QUrl heightMap READ source WRITE setSource NOTIFY sourceChanged)
#endif
};

QT_END_NAMESPACE

#endif
