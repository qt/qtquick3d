/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import Qt.labs.animation

Item {
    id: root
    property int value: 50
    property int maximumValue: 99
    width: 100
    height: 240

    DragHandler {
        id: dragHandler
        target: knob
        xAxis.enabled: false
        yAxis.minimum: slot.y
        yAxis.maximum: slot.height + slot.y - knob.height
    }

    WheelHandler {
        id: wheelHandler
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        invertible: false
        rotationScale: -0.5
        target: knob
        property: "y"
    }

    Rectangle {
        id: slot
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 10
        anchors.topMargin: 30
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
        width: 10
        color: "black"
        radius: width / 2
        smooth: true
    }

    Rectangle {
        id: knob
        objectName: "Slider Knob"
        width: parent.width - 2
        height: 30
        radius: 5
        color: "beige"
        border.width: 3
        border.color: hover.hovered ? "orange" : "black"
        property bool programmatic: false
        property real multiplier: root.maximumValue / (dragHandler.yAxis.maximum - dragHandler.yAxis.minimum)
        onYChanged: if (!programmatic) root.value = root.maximumValue - (knob.y - dragHandler.yAxis.minimum) * multiplier
        transformOrigin: Item.Center
        function setValue(value) { knob.y = dragHandler.yAxis.maximum - value / knob.multiplier }
        HoverHandler {
            id: hover
            objectName: "Slider"
        }
        BoundaryRule on y {
            id: ybr
            minimum: slot.y
            maximum: slot.height + slot.y - knob.height
        }
    }

    Component.onCompleted: {
        knob.programmatic = true
        knob.setValue(root.value)
        knob.programmatic = false
    }
}
