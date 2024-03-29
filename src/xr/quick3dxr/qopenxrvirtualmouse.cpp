// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrvirtualmouse_p.h"
#include "qopenxrview_p.h"

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtGui/QMouseEvent>

QT_BEGIN_NAMESPACE

QOpenXRVirtualMouse::QOpenXRVirtualMouse(QObject *parent) : QObject(parent)
{

}

bool QOpenXRVirtualMouse::rightMouseButton() const
{
    return m_rightMouseButton;
}

bool QOpenXRVirtualMouse::leftMouseButton() const
{
    return m_leftMouseButton;
}

bool QOpenXRVirtualMouse::middleMouseButton() const
{
    return m_middleMouseButton;
}

QQuick3DNode *QOpenXRVirtualMouse::source() const
{
    return m_source;
}

QOpenXRView *QOpenXRVirtualMouse::view() const
{
    return m_view;
}

bool QOpenXRVirtualMouse::enabled() const
{
    return m_enabled;
}

void QOpenXRVirtualMouse::setRightMouseButton(bool rightMouseButton)
{
    if (m_rightMouseButton == rightMouseButton)
        return;

    m_rightMouseButton = rightMouseButton;
    emit rightMouseButtonChanged(m_rightMouseButton);
    if (m_rightMouseButton) {
        // Press
        generateEvent(QEvent::MouseButtonPress, Qt::RightButton);
    } else {
        // Release
        generateEvent(QEvent::MouseButtonRelease, Qt::RightButton);
    }
}

void QOpenXRVirtualMouse::setLeftMouseButton(bool leftMouseButton)
{
    if (m_leftMouseButton == leftMouseButton)
        return;

    m_leftMouseButton = leftMouseButton;
    emit leftMouseButtonChanged(m_leftMouseButton);
    if (m_leftMouseButton) {
        // Press
        generateEvent(QEvent::MouseButtonPress, Qt::LeftButton);
    } else {
        // Release
        generateEvent(QEvent::MouseButtonRelease, Qt::LeftButton);
    }
}

void QOpenXRVirtualMouse::setMiddleMouseButton(bool middleMouseButton)
{
    if (m_middleMouseButton == middleMouseButton)
        return;

    m_middleMouseButton = middleMouseButton;
    emit middleMouseButtonChanged(m_middleMouseButton);
    if (m_middleMouseButton) {
        // Press
        generateEvent(QEvent::MouseButtonPress, Qt::MiddleButton);
    } else {
        // Release
        generateEvent(QEvent::MouseButtonRelease, Qt::MiddleButton);
    }
}

void QOpenXRVirtualMouse::setSource(QQuick3DNode *source)
{
    if (m_source == source)
        return;

    if (!source)
        disconnect(m_source, &QQuick3DNode::sceneTransformChanged, this, &QOpenXRVirtualMouse::moveEvent);

    m_source = source;
    emit sourceChanged(m_source);

    if (m_source)
        connect(m_source, &QQuick3DNode::sceneTransformChanged, this, &QOpenXRVirtualMouse::moveEvent);

}

void QOpenXRVirtualMouse::setView(QOpenXRView *view)
{
    if (m_view == view)
        return;

    m_view = view;
    emit viewChanged(m_view);
}

void QOpenXRVirtualMouse::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

void QOpenXRVirtualMouse::moveEvent()
{
    generateEvent(QEvent::MouseMove, Qt::NoButton);
}

void QOpenXRVirtualMouse::generateEvent(QEvent::Type type, Qt::MouseButton button)
{
    if (!m_view || !m_source || m_view->m_inDestructor)
        return;

    // Get Ray
    const QVector3D origin = m_source->scenePosition();
    const QVector3D direction = m_source->forward();

    // Generate Pointer Event
    Qt::MouseButtons buttons = Qt::NoButton;
    if (m_leftMouseButton)
        buttons |= Qt::LeftButton;
    if (m_rightMouseButton)
        buttons |= Qt::RightButton;
    if (m_middleMouseButton)
        buttons |= Qt::MiddleButton;

    // Position will be generated by QtQuick3D
    QMouseEvent *event = new QMouseEvent(type, QPointF(), QPointF(), button, buttons, Qt::NoModifier);

    // Send to View with Ray
    if (m_view->view3d()) // no internal view3D if XrView init failed but the object is still alive, handle this gracefully
        m_view->view3d()->processPointerEventFromRay(origin, direction, event);

    // Cleanup
    delete event;
}

QT_END_NAMESPACE
