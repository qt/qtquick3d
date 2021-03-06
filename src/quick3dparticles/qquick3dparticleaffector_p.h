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
#include <QtQuick3DParticles/private/qquick3dparticledata_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>
#include <QtQuick3DParticles/private/qquick3dparticleemitter_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleAffector : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QQmlListProperty<QQuick3DParticle> particles READ particles)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    QML_NAMED_ELEMENT(Affector3D)
    QML_UNCREATABLE("Affector3D is abstract")
    QML_ADDED_IN_VERSION(6, 1)

public:
    QQuick3DParticleAffector(QQuick3DNode *parent = nullptr);
    ~QQuick3DParticleAffector() override;

    QQuick3DParticleSystem* system() const;
    bool enabled() const;

    // Particles list handling
    QQmlListProperty<QQuick3DParticle> particles();
    void appendParticle(QQuick3DParticle *);
    qsizetype particleCount() const;
    QQuick3DParticle *particle(qsizetype) const;
    void clearParticles();
    void replaceParticle(qsizetype, QQuick3DParticle *);
    void removeLastParticle();

public Q_SLOTS:
    void setSystem(QQuick3DParticleSystem* system);
    void setEnabled(bool enabled);

protected:
    friend class QQuick3DParticleSystem;
    // From QQmlParserStatus
    void componentComplete() override;

    virtual void affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time) = 0;

Q_SIGNALS:
    void update();
    void systemChanged();
    void enabledChanged();

protected:
    QQuick3DParticleSystem *m_system = nullptr;
    bool m_enabled = true;

    static void appendParticle(QQmlListProperty<QQuick3DParticle> *, QQuick3DParticle *);
    static qsizetype particleCount(QQmlListProperty<QQuick3DParticle> *);
    static QQuick3DParticle *particle(QQmlListProperty<QQuick3DParticle> *, qsizetype);
    static void clearParticles(QQmlListProperty<QQuick3DParticle> *);
    static void replaceParticle(QQmlListProperty<QQuick3DParticle> *, qsizetype, QQuick3DParticle *);
    static void removeLastParticle(QQmlListProperty<QQuick3DParticle> *);
    QList<QQuick3DParticle *> m_particles;
    QMap<QQuick3DParticle *, QMetaObject::Connection> m_connections;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEAFFECTOR_H
