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

#include "inputlistview.h"

#include <QDropEvent>
#include <QMimeData>

InputListView::InputListView(QWidget *parent) : QListWidget(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAcceptDrops(true);
}

bool InputListView::tryAddItem(const QString &label)
{
    if (containsItem(label))
        return false;

    addItem(label);
    return true;
}

void InputListView::dropEvent(QDropEvent *event)
{
    constexpr int MAX_URLS = 1024;
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0; i < urlList.size() && i < MAX_URLS; ++i) {
            const QUrl &url = urlList.at(i);
            const auto filename = url.toLocalFile();
            if (url.isLocalFile() && !containsItem(filename))
                addItem(filename);
        }
    }
}

void InputListView::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void InputListView::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void InputListView::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

bool InputListView::containsItem(const QString &needle)
{
    for (int i = 0; i < count(); ++i) {
        if (item(i)->text() == needle)
            return true;
    }
    return false;
}
