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

#ifndef QQUICK3DPARTICLEMODELPARTICLE_H
#define QQUICK3DPARTICLEMODELPARTICLE_H

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

#include <QColor>
#include <QVector4D>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/qquick3dinstancing.h>

#include <QtQuick3DParticles/private/qquick3dparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>
#include <QtQuick3DParticles/private/qquick3dparticledata_p.h>

QT_BEGIN_NAMESPACE

class QQmlInstanceModel;
class QQmlChangeSet;
class QQuick3DParticleInstanceTable;

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleModelParticle : public QQuick3DParticle
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QQuick3DInstancing *instanceTable READ instanceTable NOTIFY instanceTableChanged)

    QML_NAMED_ELEMENT(ModelParticle3D)
    Q_CLASSINFO("DefaultProperty", "delegate")
    QML_ADDED_IN_VERSION(6, 2)

public:
    QQuick3DParticleModelParticle(QQuick3DNode *parent = nullptr);

    QQmlComponent *delegate() const;
    QQuick3DInstancing *instanceTable() const;

public Q_SLOTS:
    void setDelegate(QQmlComponent *delegate);

Q_SIGNALS:
    void delegateChanged();
    void instanceTableChanged();

protected:
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

    void setDepthBias(float bias) override
    {
        QQuick3DParticle::setDepthBias(bias);
        if (m_node)
            updateDepthBias(bias);
    }
private:
    void regenerate();
    void handleMaxAmountChanged(int amount);
    void handleSortModeChanged(QQuick3DParticle::SortMode mode);

    friend class QQuick3DParticleSystem;
    friend class QQuick3DParticleEmitter;

    void clearInstanceTable();
    void addInstance(const QVector3D &position, const QVector3D &scale,
                     const QVector3D &eulerRotation, const QColor &color,
                     float age);
    void commitInstance();
    void updateDepthBias(float bias);

    QPointer<QQmlComponent> m_delegate;
    QPointer<QQuick3DNode> m_node;
    QQuick3DParticleInstanceTable *m_instanceTable = nullptr;

    QVector3D m_initialScale;
    float m_initialOpacity = 1.0f;
    bool m_explicitColor;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEMODELPARTICLE_H
