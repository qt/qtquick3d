// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRGAMEPADINPUT_P_H
#define QQUICK3DXRGAMEPADINPUT_P_H

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
#include <QVector2D>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuick3DXrGamepadInput : public QObject
{
    Q_OBJECT

    QML_NAMED_ELEMENT(XrGamepadInput)
    QML_UNCREATABLE("Created by XrView")
    QML_ADDED_IN_VERSION(6, 8)

public:
    explicit QQuick3DXrGamepadInput(QObject *parent = nullptr);
    void setInputValue(int id, const char *shortName, float value) { emit inputValueChange(id, shortName, value); }

Q_SIGNALS:
    void inputValueChange(int id, const char *shortName, float value);

private:

};

QT_END_NAMESPACE

#endif // QQUICK3DXRGAMEPADINPUT_P_H
