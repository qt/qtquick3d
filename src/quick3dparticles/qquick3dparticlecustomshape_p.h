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

#ifndef QQUICK3DPARTICLECUSTOMSHAPE_H
#define QQUICK3DPARTICLECUSTOMSHAPE_H

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

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleCustomShape : public QQuick3DParticleAbstractShape
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool randomizeData READ randomizeData WRITE setRandomizeData NOTIFY randomizeDataChanged)
    QML_NAMED_ELEMENT(ParticleCustomShape3D)
    QML_ADDED_IN_VERSION(6, 3)
public:
    explicit QQuick3DParticleCustomShape(QObject *parent = nullptr);

    QUrl source() const;
    bool randomizeData() const;

    // Returns point inside this shape
    QVector3D getPosition(int particleIndex) override;

public Q_SLOTS:
    void setSource(const QUrl &source);
    void setRandomizeData(bool random);

Q_SIGNALS:
    void sourceChanged();
    void randomizeDataChanged();

private:
    void loadFromSource();
    void doRandomizeData();

    QUrl m_source;
    bool m_random = false;
    bool m_randomizeDirty = false;
    QList<QVector3D> m_positions;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLECUSTOMSHAPE_H
