// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

class InputTab;
class SettingsTab;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget = nullptr;
    InputTab *generalTab = nullptr;
    SettingsTab *settingsTab = nullptr;
};
