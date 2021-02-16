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

#include "qquick3dparticlemodelparticle_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ModelParticle3D
    \inherits Particle3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Particle using a Qt Quick 3D Model.

    The ModelParticle3D is a logical particle element that creates particles
    from a Qt Quick 3D \l Model component.
*/

QQuick3DParticleModelParticle::QQuick3DParticleModelParticle(QQuick3DNode *parent)
    : QQuick3DParticle(parent)
    , m_initialScale(1.0f, 1.0f, 1.0f)
{
    QObject::connect(this, &QQuick3DParticle::maxAmountChanged, [this]() {
        handleMaxAmountChanged(m_maxAmount);
    });

}

void QQuick3DParticleModelParticle::handleMaxAmountChanged(int maxAmount)
{
    m_particleData.clear();
    for (int i = 0; i < maxAmount; i++) {
        QQuick3DParticleData data;
        m_particleData.append(data);
    }
}

/*!
    \qmlproperty Component ModelParticle3D::delegate

    The delegate provides a template defining each object instantiated by the particle.

    For example, to allocate 200 red cube particles:

    \qml
    Component {
        id: particleComponent
        Model {
            source: "#Cube"
            scale: Qt.vector3d(0.2, 0.2, 0.2)
            materials: DefaultMaterial { }
        }
    }

    ModelParticle3D {
        id: particleRed
        delegate: particleComponent
        maxAmount: 200
        color: "#ff0000"
    }
    \endqml
*/
QQmlComponent *QQuick3DParticleModelParticle::delegate() const
{
    return m_delegate.data();
}

void QQuick3DParticleModelParticle::setDelegate(QQmlComponent *delegate)
{
    if (delegate == m_delegate)
        return;
    m_delegate = delegate;

    regenerate();
    Q_EMIT delegateChanged();
}

class QQuick3DParticleInstanceTable : public QQuick3DInstancing
{
public:
    QQuick3DParticleInstanceTable() {}
    void clear() { m_instances.clear(); }
    void commit() { markDirty(); }
    void addInstance(const QVector3D &position,
                     const QVector3D &scale, const QVector3D &eulerRotation,
                                             const QColor &color) {
        auto entry = calculateTableEntry(position, scale, eulerRotation, color);
        m_instances.append(reinterpret_cast<char *>(&entry), sizeof(InstanceTableEntry));
    }
protected:
    QByteArray getInstanceBuffer(int *instanceCount) override
    {
        if (instanceCount)
            *instanceCount = int(m_instances.count() / sizeof(InstanceTableEntry));

        return m_instances;
    }

private:
    QByteArray m_instances;
};

QQuick3DInstancing *QQuick3DParticleModelParticle::instanceTable() const
{
    return m_instanceTable;
}

void QQuick3DParticleModelParticle::clearInstanceTable()
{
    if (m_instanceTable)
        m_instanceTable->clear();
}

void QQuick3DParticleModelParticle::addInstance(const QVector3D &position, const QVector3D &scale, const QVector3D &eulerRotation, const QColor &color)
{
    if (m_instanceTable)
        m_instanceTable->addInstance(position, scale, eulerRotation, color);
}

void QQuick3DParticleModelParticle::commitInstance()
{
    if (m_instanceTable)
        m_instanceTable->commit();
}

void QQuick3DParticleModelParticle::setHasTransparency(bool transparent)
{
    if (m_instanceTable)
        m_instanceTable->setHasTransparency(transparent);
}

static void setInstancing(QQuick3DNode *node, QQuick3DInstancing *instanceTable)
{
    auto *asModel = qobject_cast<QQuick3DModel *>(node);
    if (asModel)
        asModel->setInstancing(instanceTable);
    const auto children = node->childItems();
    for (auto *child : children) {
        auto *childNode = qobject_cast<QQuick3DNode *>(child);
        if (childNode)
            setInstancing(childNode, instanceTable);
    }
}

void QQuick3DParticleModelParticle::regenerate()
{
    delete m_node;
    m_node = nullptr;

    if (!isComponentComplete())
        return;

    if (!m_instanceTable) {
        m_instanceTable = new QQuick3DParticleInstanceTable();
        m_instanceTable->setParent(this);
        m_instanceTable->setParentItem(this);
        emit instanceTableChanged();
    } else {
        m_instanceTable->clear();
    }

    if (m_delegate.isNull())
        return;

    auto *obj = m_delegate->create(m_delegate->creationContext());

    m_node = qobject_cast<QQuick3DNode *>(obj);
    if (m_node) {
        setInstancing(m_node, m_instanceTable);
        auto *particleSystem = system();
        m_node->setParent(particleSystem);
        m_node->setParentItem(particleSystem);
    } else {
        delete obj;
    }
}

void QQuick3DParticleModelParticle::componentComplete()
{
    if (!system() && qobject_cast<QQuick3DParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QQuick3DParticleSystem*>(parentItem()));

    QQuick3DParticle::componentComplete();
    regenerate();
}

void QQuick3DParticleModelParticle::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    QQuick3DObject::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        regenerate();
    }
}

void QQuick3DParticleModelParticle::reset() {
    m_currentIndex = 0;
    m_lastBurstIndex = 0;

    // Reset all particles data
    QQuick3DParticleData data;
    for (int i = 0; i < m_particleData.size(); i++)
        m_particleData[i] = data;
}

QT_END_NAMESPACE
