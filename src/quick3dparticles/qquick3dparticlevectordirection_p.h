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

#ifndef QQUICK3DPARTICLEVECTORDIRECTION_H
#define QQUICK3DPARTICLEVECTORDIRECTION_H

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

#include <QVector3D>
#include <QtQuick3DParticles/private/qquick3dparticledirection_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleVectorDirection : public QQuick3DParticleDirection
{
    Q_OBJECT
    Q_PROPERTY(QVector3D direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QVector3D directionVariation READ directionVariation WRITE setDirectionVariation NOTIFY directionVariationChanged)
    QML_NAMED_ELEMENT(VectorDirection3D)

public:
    QQuick3DParticleVectorDirection(QObject *parent = nullptr);

    QVector3D direction() const;
    QVector3D directionVariation() const;

public Q_SLOTS:
    void setDirection(const QVector3D &direction);
    void setDirectionVariation(const QVector3D &directionVariation);

Q_SIGNALS:
    void directionChanged();
    void directionVariationChanged();

private:
    QVector3D sample(const QQuick3DParticleData &d) override;
    QVector3D m_direction;
    QVector3D m_directionVariation;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEVECTORDIRECTION_H
