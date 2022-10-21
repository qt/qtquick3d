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

#ifndef QQUICK3DPARTICLESHAPE_H
#define QQUICK3DPARTICLESHAPE_H

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

#include "qquick3dparticleabstractshape_p.h"
#include <QVector3D>

QT_BEGIN_NAMESPACE

class QQuick3DParticleSystem;
class QQuick3DModel;

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleShape : public QQuick3DParticleAbstractShape
{
    Q_OBJECT
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY fillChanged)
    Q_PROPERTY(ShapeType type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QVector3D extents READ extents WRITE setExtents NOTIFY extentsChanged)

    QML_NAMED_ELEMENT(ParticleShape3D)
    QML_ADDED_IN_VERSION(6, 2)

public:
    enum ShapeType
    {
        Cube = 0,
        Sphere,
        Cylinder
    };
    Q_ENUM(ShapeType)

    QQuick3DParticleShape(QObject *parent = nullptr);

    bool fill() const;
    ShapeType type() const;
    QVector3D extents() const;

    // Returns point inside this shape
    QVector3D getPosition(int particleIndex) override;

public Q_SLOTS:
    void setFill(bool fill);
    void setType(QQuick3DParticleShape::ShapeType type);
    void setExtents(QVector3D extends);

Q_SIGNALS:
    void fillChanged();
    void typeChanged();
    void extentsChanged();

private:
    QVector3D randomPositionCube(int particleIndex) const;
    QVector3D randomPositionSphere(int particleIndex) const;
    QVector3D randomPositionCylinder(int particleIndex) const;

    bool m_fill = true;
    ShapeType m_type = ShapeType::Cube;
    QVector3D m_extents = QVector3D(50, 50, 50);
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLESHAPE_H
