// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRVIRTUALMOUSE_H
#define QOPENXRVIRTUALMOUSE_H

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
#include <QEvent>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QOpenXRView;
class QQuick3DNode;

class QOpenXRVirtualMouse : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool rightMouseButton READ rightMouseButton WRITE setRightMouseButton NOTIFY rightMouseButtonChanged)
    Q_PROPERTY(bool leftMouseButton READ leftMouseButton WRITE setLeftMouseButton NOTIFY leftMouseButtonChanged)
    Q_PROPERTY(bool middleMouseButton READ middleMouseButton WRITE setMiddleMouseButton NOTIFY middleMouseButtonChanged)
    Q_PROPERTY(QQuick3DNode* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QOpenXRView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    QML_NAMED_ELEMENT(XrVirtualMouse)

public:
    explicit QOpenXRVirtualMouse(QObject *parent = nullptr);

    bool rightMouseButton() const;
    bool leftMouseButton() const;
    bool middleMouseButton() const;

    QQuick3DNode *source() const;
    QOpenXRView* view() const;

    bool enabled() const;

public Q_SLOTS:
    void setRightMouseButton(bool rightMouseButton);
    void setLeftMouseButton(bool leftMouseButton);
    void setMiddleMouseButton(bool middleMouseButton);
    void setSource(QQuick3DNode* source);
    void setView(QOpenXRView* view);
    void setEnabled(bool enabled);

private Q_SLOTS:
    void moveEvent();

Q_SIGNALS:
    void rightMouseButtonChanged(bool rightMouseButton);
    void leftMouseButtonChanged(bool leftMouseButton);
    void middleMouseButtonChanged(bool middleMouseButton);
    void sourceChanged(QQuick3DNode* source);
    void viewChanged(QOpenXRView* view);
    void enabledChanged(bool enabled);

private:
    void generateEvent(QEvent::Type type, Qt::MouseButton button = Qt::NoButton);

    bool m_rightMouseButton = false;
    bool m_leftMouseButton = false;
    bool m_middleMouseButton = false;
    QQuick3DNode* m_source = nullptr;
    QOpenXRView* m_view = nullptr;
    bool m_enabled = true;
};

QT_END_NAMESPACE

#endif // QOPENXRVIRTUALMOUSE_H
