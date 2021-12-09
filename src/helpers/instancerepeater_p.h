/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef INSTANCEREPEATER_P_H
#define INSTANCEREPEATER_P_H

#include <QtQuick3D/qquick3dinstancing.h>
#include <QtQuick3D/private/qquick3drepeater_p.h>
#include <QAbstractListModel>

// Workaround for QTBUG-94099, ensures qml_register_types...() is exported
#include "qtquick3dhelpersglobal_p.h"

QT_BEGIN_NAMESPACE

class InstanceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DInstancing *instancingTable READ instancing WRITE setInstancing NOTIFY instancingChanged)
    QML_NAMED_ELEMENT(InstanceModel)
    QML_ADDED_IN_VERSION(6, 4)

public:
    explicit InstanceModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;

    QQuick3DInstancing *instancing() const { return m_instancing; }
    void setInstancing(QQuick3DInstancing *instancing);

    const QQuick3DInstancing::InstanceTableEntry *instanceData(int index) const;

    enum Roles {
        PositionRole, RotationRole, ScaleRole, ColorRole, CustomDataRole
    };

    QHash<int, QByteArray> roleNames() const override {
        return {
            { ColorRole, "modelColor" },
            { PositionRole, "modelPosition" },
            { RotationRole, "modelRotation"},
            { ScaleRole, "modelScale"},
            { CustomDataRole, "modelData"}
        };
    }

private slots:
    void reset();

signals:
    void instancingChanged();

private:
    void ensureTable() const;

    QQuick3DInstancing *m_instancing = nullptr;

    QByteArray m_instanceData;
    int m_count = 0;
    QMetaObject::Connection m_tableConnection;
};

class InstanceRepeater : public QQuick3DRepeater
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DInstancing *instancingTable READ instancing WRITE setInstancing NOTIFY instancingChanged)
    QML_NAMED_ELEMENT(InstanceRepeater)
    QML_ADDED_IN_VERSION(6, 4)

public:
    explicit InstanceRepeater(QQuick3DNode *parent = nullptr);
    QQuick3DInstancing *instancing() const;
    void setInstancing(QQuick3DInstancing *instancing);
signals:
    void instancingChanged();
protected:
    void initDelegate(int index, QQuick3DNode *node) override;
private:
    InstanceModel *m_model = nullptr;
};
QT_END_NAMESPACE
#endif // INSTANCEREPEATER_P_H
