// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrvirtualmouse_p.h"
#include "qquick3dxrview_p.h"

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtGui/QMouseEvent>

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrVirtualMouse
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Represents a virtual mouse for interacting with UIs in XR.
*/

QQuick3DXrVirtualMouse::QQuick3DXrVirtualMouse(QObject *parent) : QObject(parent)
{

}


/*!
    \qmlproperty bool XrVirtualMouse::rightMouseButton
    \brief Indicates whether the right mouse button is pressed.

    When set to true, the right mouse button is considered pressed.
*/

bool QQuick3DXrVirtualMouse::rightMouseButton() const
{
    return m_rightMouseButton;
}

/*!
    \qmlproperty bool XrVirtualMouse::leftMouseButton
    \brief Indicates whether the left mouse button is pressed.

    When set to true, the left mouse button is considered pressed.
*/

bool QQuick3DXrVirtualMouse::leftMouseButton() const
{
    return m_leftMouseButton;
}

/*!
    \qmlproperty bool XrVirtualMouse::middleMouseButton
    \brief Indicates whether the middle mouse button is pressed.

    When set to true, the middle mouse button is considered pressed.
*/

bool QQuick3DXrVirtualMouse::middleMouseButton() const
{
    return m_middleMouseButton;
}

/*!
    \qmlproperty Node XrVirtualMouse::source
    \brief The 3D node associated with the virtual mouse.
    Holds a node representing the position of the virtual cursor in the 3D scene.
*/

QQuick3DNode *QQuick3DXrVirtualMouse::source() const
{
    return m_source;
}

/*!
    \qmlproperty XrView XrVirtualMouse::view
    \brief The OpenXR view associated with the virtual mouse.
    Holds the view in which the virtual mouse operates.
*/

QQuick3DXrView *QQuick3DXrVirtualMouse::view() const
{
    return m_view;
}

/*!
    \qmlproperty bool XrVirtualMouse::enabled
    \brief Indicates whether the virtual mouse is enabled.
    When true, the virtual mouse can interact with 3D objects in the scene.
*/

bool QQuick3DXrVirtualMouse::enabled() const
{
    return m_enabled;
}

void QQuick3DXrVirtualMouse::setRightMouseButton(bool rightMouseButton)
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

void QQuick3DXrVirtualMouse::setLeftMouseButton(bool leftMouseButton)
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

void QQuick3DXrVirtualMouse::setMiddleMouseButton(bool middleMouseButton)
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

void QQuick3DXrVirtualMouse::setSource(QQuick3DNode *source)
{
    if (m_source == source)
        return;

    if (!source)
        disconnect(m_source, &QQuick3DNode::sceneTransformChanged, this, &QQuick3DXrVirtualMouse::moveEvent);

    m_source = source;
    emit sourceChanged(m_source);

    if (m_source)
        connect(m_source, &QQuick3DNode::sceneTransformChanged, this, &QQuick3DXrVirtualMouse::moveEvent);

}

void QQuick3DXrVirtualMouse::setView(QQuick3DXrView *view)
{
    if (m_view == view)
        return;

    m_view = view;
    emit viewChanged(m_view);
}

void QQuick3DXrVirtualMouse::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    emit enabledChanged(m_enabled);

    // Generate mouse release event if we disable when pressed
    if (!m_enabled && (m_leftMouseButton || m_rightMouseButton || m_middleMouseButton)) {
        Qt::MouseButton button = m_leftMouseButton ? Qt::LeftButton : m_rightMouseButton ? Qt::RightButton : Qt::MiddleButton;
        m_leftMouseButton = m_rightMouseButton = m_middleMouseButton = false;
        generateEvent(QEvent::MouseButtonRelease, button);
    }
}

void QQuick3DXrVirtualMouse::moveEvent()
{
    generateEvent(QEvent::MouseMove, Qt::NoButton);
}

void QQuick3DXrVirtualMouse::generateEvent(QEvent::Type type, Qt::MouseButton button)
{
    if (!m_view || !m_source || m_view->m_inDestructor || !m_enabled)
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


/*! \qmlsignal XrVirtualMouse::rightMouseButtonChanged(bool rightMouseButton)
    \brief Emitted when the state of the right mouse button changes.
    \a rightMouseButton has new state of the right mouse button
    (true if pressed, false if released).
*/
    /*! \qmlsignal XrVirtualMouse::leftMouseButtonChanged(bool leftMouseButton)
    \brief Emitted when the state of the left mouse button changes.
    \a leftMouseButton has new state of the left mouse button
    (true if pressed, false if released).
*/
/*! \qmlsignal XrVirtualMouse::middleMouseButtonChanged(bool middleMouseButton)
    \brief Emitted when the state of the middle mouse button changes.
    \a middleMouseButton has new state of the middle mouse button
    (true if pressed, false if released).
*/
/*! \qmlsignal XrVirtualMouse::sourceChanged(Node source);
    \brief Emitted when the source property changes.
    \a source has the new Node held in the source property.
*/
/*! \qmlsignal XrVirtualMouse::viewChanged(XrView view);
    \brief Emitted when the view property changes.
    \a view has the new XrView held in the view property.
*/
/*! \qmlsignal XrVirtualMouse::enabledChanged(bool enabled);
    \brief Emitted when the enabled property changes.
    \a enabled has the new value of the enabled property.
*/

QT_END_NAMESPACE