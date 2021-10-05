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

#ifndef UNIFORMMODEL_H
#define UNIFORMMODEL_H

#include <QAbstractTableModel>
#include <QtQml/qqmlregistration.h>
#include "custommaterial.h"

QT_BEGIN_NAMESPACE

class UniformModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    using UniformType = CustomMaterial::Uniform::Type;
    Q_ENUM(UniformType)
    using UniformTable = CustomMaterial::UniformTable;

    enum UniformModelRoles {
        Type = Qt::UserRole + 1,
        Name,
        Value
    };

    explicit UniformModel(QObject *parent = nullptr);

    void setModelData(UniformTable *data);
    int rowCount(const QModelIndex & = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role) const final;
    QHash<int, QByteArray> roleNames() const final;
    bool setData(const QModelIndex &index, const QVariant &value, int role) final;

    Q_INVOKABLE bool insertRow(int rowIndex, int type, const QString &id);
    Q_INVOKABLE void removeRow(int rowIndex, int rows = 1);

private:
    bool validateUniformName(const QString &uniformName);
    UniformTable *m_uniformTable = nullptr;
};

QT_END_NAMESPACE

#endif // UNIFORMMODEL_H
