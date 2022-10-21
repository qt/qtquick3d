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
    QML_ADDED_IN_VERSION(6, 2)
    Q_PROPERTY(int instanceCountOverride READ instanceCountOverride WRITE setInstanceCountOverride NOTIFY instanceCountOverrideChanged)
    Q_PROPERTY(bool hasTransparency READ hasTransparency WRITE setHasTransparency NOTIFY hasTransparencyChanged)
    Q_PROPERTY(bool depthSortingEnabled READ depthSortingEnabled WRITE setDepthSortingEnabled NOTIFY depthSortingEnabledChanged)

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

    QByteArray instanceBuffer(int *instanceCount);
    int instanceCountOverride() const;
    bool hasTransparency() const;
    bool depthSortingEnabled() const;

public Q_SLOTS:
    void setInstanceCountOverride(int instanceCountOverride);
    void setHasTransparency(bool hasTransparency);
    void setDepthSortingEnabled(bool enabled);

Q_SIGNALS:
    void instanceNodeDirty();
    void instanceCountOverrideChanged();
    void hasTransparencyChanged();
    void depthSortingEnabledChanged();

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

QT_END_NAMESPACE

#endif // Q_QUICK3D_INSTANCING_H
