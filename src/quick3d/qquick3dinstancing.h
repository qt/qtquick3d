/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef Q_QUICK3D_INSTANCING_H
#define Q_QUICK3D_INSTANCING_H

#include <QtQuick3D/qquick3dobject.h>
#include <QtGui/QVector4D>
#include <QtGui/QQuaternion>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE

class QQuick3DInstancingPrivate;

class Q_QUICK3D_EXPORT QQuick3DInstancing : public QQuick3DObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuick3DInstancing)

    QML_NAMED_ELEMENT(Instancing)
    QML_UNCREATABLE("Instancing is Abstract")
    //QML_ADDED_IN_VERSION(6, 1)
    Q_PROPERTY(int instanceCountOverride READ instanceCountOverride WRITE setInstanceCountOverride NOTIFY instanceCountOverrideChanged)

public:
    struct InstanceTableEntry {
        QVector4D row0;
        QVector4D row1;
        QVector4D row2;
        QVector4D color;
        QVector4D instanceData;
    };

    explicit QQuick3DInstancing(QQuick3DObject *parent = nullptr);
    ~QQuick3DInstancing() override;



//#### TODO: must have bounds somehow ...and later support instance picking

    QByteArray instanceBuffer(int *instanceCount);
    int instanceCountOverride() const;

public Q_SLOTS:
    void setInstanceCountOverride(int instanceCountOverride);

Q_SIGNALS:
    void instanceNodeDirty();

    void instanceCountOverrideChanged(int instanceCountOverride);

protected:
    virtual QByteArray getInstanceBuffer(int *instanceCount) = 0;
    void markDirty();
    static InstanceTableEntry calculateTableEntry(const QVector3D &position,
                          const QVector3D &scale, const QVector3D &eulerRotation,
                                                  const QColor &color, const QVector4D &customData = {});
    static InstanceTableEntry calculateTableEntryFromQuaternion(const QVector3D &position,
                          const QVector3D &scale, const QQuaternion &rotation,
                                                  const QColor &color, const QVector4D &customData = {});
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
};

class Q_QUICK3D_EXPORT QQuick3DInstanceListEntry : public QQuick3DObject
{
    Q_OBJECT

    //QML_ADDED_IN_VERSION(6, 1)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(QVector3D eulerRotation READ eulerRotation WRITE setEulerRotation NOTIFY eulerRotationChanged)
    Q_PROPERTY(QQuaternion rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QVector4D customData READ customData WRITE setCustomData NOTIFY customDataChanged)
    QML_NAMED_ELEMENT(InstanceListEntry)

public:
    explicit QQuick3DInstanceListEntry(QQuick3DObject *parent = nullptr);
    ~QQuick3DInstanceListEntry() override {}

    QVector3D position() const
    {
        return m_position;
    }
    QVector3D scale() const
    {
        return m_scale;
    }

    QVector3D eulerRotation() const
    {
        return m_eulerRotation;
    }

    QQuaternion rotation() const
    {
        return m_rotation;
    }

    QColor color() const
    {
        return m_color;
    }

    QVector4D customData() const
    {
        return m_customData;
    }

public Q_SLOTS:
    void setPosition(QVector3D position);
    void setScale(QVector3D scale);
    void setEulerRotation(QVector3D eulerRotation);
    void setRotation(QQuaternion rotation);
    void setColor(QColor color);
    void setCustomData(QVector4D customData);

Q_SIGNALS:
    void positionChanged(QVector3D position);
    void scaleChanged(QVector3D scale);
    void eulerRotationChanged(QVector3D eulerRotation);
    void rotationChanged(QQuaternion rotation);
    void colorChanged(QColor color);
    void customDataChanged(QVector4D customData);
    void changed();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *) override
    {
        return nullptr;
    }

private:
    QVector3D m_position;
    QVector3D m_scale = {1, 1, 1};
    QVector3D m_eulerRotation;
    QQuaternion m_rotation;
    QColor m_color = Qt::white;
    QVector4D m_customData;
    bool m_useEulerRotation = true;
    friend class QQuick3DInstanceList;
};

class Q_QUICK3D_EXPORT QQuick3DInstanceList : public QQuick3DInstancing
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuick3DInstanceListEntry> instances READ instances)
    QML_NAMED_ELEMENT(InstanceList)
    //QML_ADDED_IN_VERSION(6, 1)

public:
    explicit QQuick3DInstanceList(QQuick3DObject *parent = nullptr);
    ~QQuick3DInstanceList() override;

    QByteArray getInstanceBuffer(int *instanceCount) override;
    QQmlListProperty<QQuick3DInstanceListEntry> instances();

private Q_SLOTS:
    void handleInstanceChange();

private:
    void generateInstanceData();

    static void qmlAppendInstanceListEntry(QQmlListProperty<QQuick3DInstanceListEntry> *list, QQuick3DInstanceListEntry *material);
    static QQuick3DInstanceListEntry *qmlInstanceListEntryAt(QQmlListProperty<QQuick3DInstanceListEntry> *list, qsizetype index);
    static qsizetype qmlInstanceListEntriesCount(QQmlListProperty<QQuick3DInstanceListEntry> *list);
    static void qmlClearInstanceListEntries(QQmlListProperty<QQuick3DInstanceListEntry> *list);

    bool m_dirty = true;
    QByteArray m_instanceData;
    QList<QQuick3DInstanceListEntry *> m_instances;
};

QT_END_NAMESPACE

#endif // Q_QUICK3D_INSTANCING_H
