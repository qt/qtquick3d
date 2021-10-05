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

import QtQuick

Item {
    id: root
    required property Item backgroundItem
    property alias range: glassEffect.range
    property alias blur: glassEffect.blur
    property alias color: glassEffect.color
    property alias backgroundRect: backgroundSourceImage.sourceRect

    ShaderEffectSource {
        anchors.fill: parent
        id: backgroundSourceImage
        sourceRect: Qt.rect(0, 0, width, height)
        sourceItem: targetView
        visible: false
    }


    ShaderEffectSource {
        anchors.fill: parent
        id: noiseImageSource
        sourceRect: Qt.rect(0, 0, width, height)
        sourceItem: noiseImage
        visible: false
    }

    Image {
        anchors.fill: parent
        id: noiseImage
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        visible: false
        source: "qrc:/assets/images/noise.png"
    }

    ShaderEffect {
        id: glassEffect
        property variant sourceTex: backgroundSourceImage
        property variant noiseTex: noiseImageSource
        property real range: 0.25;
        property real blur: 0.05;
        property color color: "white"
        anchors.fill: parent
        fragmentShader: "qrc:/assets/shaders/frostedGlass.frag.qsb"
    }
}
