/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef SETTINGSTAB_H
#define SETTINGSTAB_H

#include <QString>
#include <QScrollArea>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

struct Setting
{
    QWidget *uiELement = nullptr;
    QString name;
    bool defaultBool = false;
    double defaultReal = 0.0;
};

class SettingsTab : public QScrollArea
{
    Q_OBJECT

public:
    explicit SettingsTab(QWidget *parent = nullptr);

    QVariantMap getOptions() const;

private:
    QList<Setting> settings;
};

#endif
