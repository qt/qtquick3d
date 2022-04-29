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

#ifndef INPUTTAB_H
#define INPUTTAB_H

#include "converterthread.h"

#include <QWidget>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

class InputListView;
class SettingsTab;

class InputTab : public QWidget
{
    Q_OBJECT

public:
    explicit InputTab(SettingsTab *settings, QWidget *parent = nullptr);

    QStringList getInputFiles() const;
    QString getOutputPath() const;

    void convert();

private slots:
    void browseInput();
    void browseOutput();
    void removeSelected();
    void selectAll();

    void convertStart(const QString &text);
    void convertUpdate(const QString &text);
    void convertDone(const QString &text);

private:
    SettingsTab *settingsTab = nullptr;
    QLineEdit *outputPathEdit = nullptr;
    InputListView *inputFilesListBox = nullptr;
    QPushButton *convertButton = nullptr;
    QPlainTextEdit *statusText = nullptr;

    QString lastInputPath;
    ConverterThread converterThread;
};

#endif
