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

#ifndef QQUICK3DPARTICLEAFFECTOR_H
#define QQUICK3DPARTICLEAFFECTOR_H

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

#include <QObject>
#include <QList>
#include <QQmlListProperty>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include "qquick3dparticledata_p.h"
#include "qquick3dparticlesystem_p.h"
#include "qquick3dparticleemitter_p.h"

QT_BEGIN_NAMESPACE

class QQuick3DParticleAffector : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    // When set, affect only these particles. If not set, affects all particles.
    // TODO: Should this be particle or emitter? Currently we don't know which emitter launched particle,
    // that would need data addition.
    Q_PROPERTY(QQmlListProperty<QQuick3DNode> particles READ particles)
    // TODO: Is startTime & endTime worth the API?
    Q_PROPERTY(int startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(int endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    QML_NAMED_ELEMENT(Affector3D)
    QML_UNCREATABLE("Affector3D is abstract")
    Q_INTERFACES(QQmlParserStatus)

public:
    QQuick3DParticleAffector(QObject *parent = nullptr);
    ~QQuick3DParticleAffector() override;

    QQuick3DParticleSystem* system() const;
    int startTime() const;
    int endTime() const;

    // Particles list handling
    QQmlListProperty<QQuick3DNode> particles();
    void appendParticle(QQuick3DNode*);
    qsizetype particleCount() const;
    QQuick3DNode *particle(qsizetype) const;
    void clearParticles();
    void replaceParticle(qsizetype, QQuick3DNode*);
    void removeLastParticle();

public Q_SLOTS:
    void setSystem(QQuick3DParticleSystem* system);
    void setStartTime(int startTime);
    void setEndTime(int endTime);

protected:
    friend class QQuick3DParticleSystem;
    // From QQmlParserStatus
    void componentComplete() override;
    void classBegin() override {}

    virtual void affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time) = 0;
    virtual bool shouldAffect(const QQuick3DParticleData &sd, float time);
    float particleTime(float time);

Q_SIGNALS:
    void update();
    void systemChanged();
    void startTimeChanged();
    void endTimeChanged();

protected:
    QQuick3DParticleSystem* m_system = nullptr;

    // By default starts immediately and never ends
    int m_startTime = 0;
    int m_endTime = AFFECTOR_MAX_TIME;

    static void appendParticle(QQmlListProperty<QQuick3DNode>*, QQuick3DNode*);
    static qsizetype particleCount(QQmlListProperty<QQuick3DNode>*);
    static QQuick3DNode* particle(QQmlListProperty<QQuick3DNode>*, qsizetype);
    static void clearParticles(QQmlListProperty<QQuick3DNode>*);
    static void replaceParticle(QQmlListProperty<QQuick3DNode>*, qsizetype, QQuick3DNode*);
    static void removeLastParticle(QQmlListProperty<QQuick3DNode>*);
    QList<QQuick3DNode *> m_particles;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEAFFECTOR_H
