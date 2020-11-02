/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick3D
import QtQuick.Particles

Window {
    id: mainWindow

    //! [main properties]
    // Scaling helpper
    readonly property real px: 0.2 + Math.min(width, height) / 800
    // This is false until the first game has started
    property bool playingStarted: false
    // This is true whenever game is on
    property bool gameOn: false
    // Sizes of our 3D models
    readonly property real ballSize: 40
    readonly property real targetSize: 120
    // Playing time in seconds
    readonly property real gameTime: 60
    property real currentTime: 0
    // Amount of balls per game
    readonly property int gameBalls: 20
    property int currentBalls: 0
    // Scores
    property int score: 0
    property int timeBonus: 0
    property int ballsBonus: 0
    //! [main properties]

    width: 800
    height: 600
    visible: true
    title: qsTr("Quick3D Quick Ball")
    color: "#000000"

    View3D {
        id: view3D
        anchors.fill: parent

        //! [view functions]
        function createLevel1() {
            // Simple level of target items
            var level1 = [{ "x": 0, "y": 100, "z": -100, "points": 10 },
                          { "x": -300, "y": 100, "z": -400, "points": 10 },
                          { "x": 300, "y": 100, "z": -400, "points": 10 },
                          { "x": -200, "y": 400, "z": -600, "points": 20 },
                          { "x": 0, "y": 400, "z": -600, "points": 20 },
                          { "x": 200, "y": 400, "z": -600, "points": 20 },
                          { "x": 0, "y": 700, "z": -600, "points": 30 }];
            targetsNode.addTargets(level1);
        }

        function startGame() {
            ballModel.resetBall();
            targetsNode.resetTargets();
            createLevel1();
            score = timeBonus = ballsBonus = 0;
            currentBalls = gameBalls;
            gameOn = true;
            playingStarted = true;
        }

        function endGame() {
            if (targetsNode.currentTargets == 0) {
                // If we managed to get all targets down -> bonus points!
                timeBonus = currentTime;
                ballsBonus = currentBalls * 10;
            }
            gameOn = false;
        }
        //! [view functions]

        environment: SceneEnvironment {
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        camera: viewCamera

        PerspectiveCamera {
            id: viewCamera
            position: Qt.vector3d(0, 200, 800);

            // Rotate camera a bit
            SequentialAnimation on eulerRotation.y {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 2
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: -2
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
            }
        }

        //! [lights]
        PointLight {
            x: 400
            y: 1200
            castsShadow: true
            shadowMapQuality: Light.ShadowMapQualityHigh
            shadowFactor: 50
            quadraticFade: 2
            ambientColor: "#202020"
            brightness: gameOn ? 200 : 40
            Behavior on brightness {
                NumberAnimation {
                    duration: 1000
                    easing.type: Easing.InOutQuad
                }
            }
        }
       //! [lights]

        //! [ball handling]
        MouseArea {
            anchors.fill: parent
            enabled: gameOn && !ballModel.ballMoving
            onPressed: {
                ballModel.moveBall(mouseX, mouseY);
            }
            onPositionChanged: {
                ballModel.moveBall(mouseX, mouseY);
            }
            onReleased: {
                ballModel.throwBall();
            }
        }
        //! [ball handling]

        //! [ball model]
        Model {
            id: ballModel
            property real directionX: 0
            property real directionY: 0
            // How many ms the ball flies
            readonly property real speed: 2000
            readonly property real ballScale: ballSize / 100
            property var moves: []
            readonly property int maxMoves: 5
            readonly property bool ballMoving: ballAnimation.running

            source: "#Sphere"
            scale: Qt.vector3d(ballScale, ballScale, ballScale)

            materials: DefaultMaterial {
                diffuseMap: Texture {
                    source: "images/ball.jpg"
                }
                normalMap: Texture {
                    source: "images/ball_n.jpg"
                }
                bumpAmount: 1.0
            }
            //! [ball model]

            //! [ball functions]
            function resetBall() {
                moves = [];
                x = 0;
                y = ballSize/2;
                z = 400;
            }

            function moveBall(posX, posY) {
                var pos = view3D.mapTo3DScene(Qt.vector3d(posX, posY, ballModel.z + ballSize));
                pos.y = Math.max(ballSize / 2, pos.y);
                var point = {"x": pos.x, "y": pos.y };
                moves.push(point);
                if (moves.length > maxMoves) moves.shift();
                // Apply position into ball model
                ballModel.x = pos.x;
                ballModel.y = pos.y;
            }

            function throwBall() {
                currentBalls--;
                var moveX = 0;
                var moveY = 0;
                if (moves.length >= 2) {
                    var first = moves.shift();
                    var last = moves.pop();
                    moveX = last.x - first.x;
                    moveY = last.y - first.y;
                    if (moveY < 0) moveY = 0;
                }
                directionX = moveX * 20;
                directionY = moveY * 4;
                ballAnimation.start();
            }
            //! [ball functions]

            //! [ball animations]
            ParallelAnimation {
                id: ballAnimation
                running: false
                // Move forward
                NumberAnimation {
                    target: ballModel
                    property: "z"
                    duration: ballModel.speed
                    to: -ballModel.directionY * 5
                    easing.type: Easing.OutQuad
                }
                // Move up & down with a bounce
                SequentialAnimation {
                    NumberAnimation {
                        target: ballModel
                        property: "y"
                        duration: ballModel.speed * (1 / 3)
                        to: ballModel.y + ballModel.directionY
                        easing.type: Easing.OutQuad
                    }
                    NumberAnimation {
                        target: ballModel
                        property: "y"
                        duration: ballModel.speed * (2 / 3)
                        to: ballSize / 4
                        easing.type: Easing.OutBounce
                    }
                }
                // Move sideways
                NumberAnimation {
                    target: ballModel
                    property: "x"
                    duration: ballModel.speed
                    to: ballModel.x + ballModel.directionX
                }

                onFinished: {
                    if (currentBalls <= 0)
                        view3D.endGame();
                    ballModel.resetBall();
                }
            }

            NumberAnimation on eulerRotation.z {
                running: ballModel.ballMoving
                loops: Animation.Infinite
                from: ballModel.directionX < 0 ? 0 : 720
                to: 360
                duration: 10000 / (2 + Math.abs(ballModel.directionX * 0.05))
            }
            //! [ball animations]

            //! [ball collisions]
            onZChanged: {
                // Loop through target items and detect collisions
                var hitMargin = ballSize / 2 + targetSize / 2;
                for (var i = 0; i < targetsNode.targets.length; ++i) {
                    var target = targetsNode.targets[i];
                    var targetPos = target.scenePosition;
                    var hit = ballModel.scenePosition.fuzzyEquals(targetPos, hitMargin);
                    if (hit) {
                        target.hit();
                        if (targetsNode.currentTargets <= 0)
                            view3D.endGame();
                    }
                }
            }
            //! [ball collisions]
        }

        //! [targets node]
        Node {
            id: targetsNode

            property var targets: []
            property int currentTargets: 0

            function addTargets(items) {
                items.forEach(function (item) {
                    let instance = targetComponent.createObject(
                            targetsNode, { "x": item.x, "y": item.y, "z": item.z, "points": item.points});
                    targets.push(instance);
                });
                currentTargets = targets.length;
            }

            function removeTarget(item) {
                var index = targets.indexOf(item);
                targets.splice(index, 1);
                currentTargets = targets.length;
            }

            function resetTargets() {
                while (targets.length > 0)
                    targets.pop().destroy();
                currentTargets = targets.length;
            }
            SequentialAnimation on y {
                running: gameOn
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0
                    to: 150
                    duration: 3000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
            }
        }
        //! [targets node]

        //! [target component]
        Component {
            id: targetComponent
            Node {
                id: targetNode

                property int points: 0
                property real hide: 0

                function hit() {
                    targetsNode.removeTarget(this);
                    score += points;
                    hitAnimation.start();
                }

                SequentialAnimation {
                    id: hitAnimation
                    NumberAnimation {
                        target: targetNode
                        property: "hide"
                        to: 1
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                    ScriptAction {
                        script: targetNode.destroy();
                    }
                }

                Model {
                    id: targetModel

                    readonly property real targetScale: (1 + hide * 2) * (targetSize / 100)

                    source: "#Cube"
                    scale: Qt.vector3d(targetScale, targetScale, targetScale)
                    opacity: 1 - hide * 2
                    materials: DefaultMaterial {
                        diffuseMap: Texture {
                            source: "images/qt_logo.jpg"
                        }
                        normalMap: Texture {
                            source: "images/qt_logo_n.jpg"
                        }
                        bumpAmount: 1.0
                    }
                    Vector3dAnimation on eulerRotation {
                        loops: Animation.Infinite
                        duration: 5000
                        from: Qt.vector3d(0, 0, 0)
                        to: Qt.vector3d(360, 360, 360)
                    }
                }
                Text {
                    anchors.centerIn: parent
                    scale: 1 + hide * 2
                    opacity: 3 - 3 * hide;
                    text: targetNode.points
                    font.pixelSize: 60 * px
                    color: "#808000"
                    style: Text.Outline
                    styleColor: "#f0f000"
                }
            }
        }
        //! [target component]

        //! [ground model]
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(50, 50, 1)
            eulerRotation.x: -90
            materials: DefaultMaterial {
                diffuseMap: Texture {
                    source: "images/grass.jpg"
                    tilingModeHorizontal: Texture.Repeat
                    tilingModeVertical: Texture.Repeat
                    scaleU: 25.0
                    scaleV: 25.0
                }
                normalMap: Texture {
                    source: "images/grass_n.jpg"
                }
                bumpAmount: 0.6
            }
        }
        //! [ground model]

        //! [sky model]
        Model {
            id: sky
            property real scaleX: 100
            property real scaleY: 20
            source: "#Rectangle"
            scale: Qt.vector3d(sky.scaleX, sky.scaleY, 1)
            position: Qt.vector3d(0, 960, -2000)
            // We don't want shadows casted into sky
            receivesShadows: false
            materials: DefaultMaterial {
                diffuseMap: Texture {
                    source: "images/sky.jpg"
                }
            }
            // Star particles
            Node {
                z: 500
                y: 30
                // Stars are far away, scale up to half the resolution
                scale: Qt.vector3d(2 / sky.scaleX, 2 / sky.scaleY, 1)
                ParticleSystem {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    width: 3000
                    height: 400
                    ImageParticle {
                        source: "qrc:///particleresources/star.png"
                        rotationVariation: 360
                        color: "#ffffa0"
                        colorVariation: 0.1
                    }
                    Emitter {
                        anchors.fill: parent
                        emitRate: 4
                        lifeSpan: 6000
                        lifeSpanVariation: 4000
                        size: 30
                        sizeVariation: 20
                    }
                }
            }
        }
        //! [sky model]
    }

    // Game time counter
    NumberAnimation on currentTime {
        running: gameOn
        duration: gameTime * 1000
        from: gameTime
        to: 0
        onFinished: {
            view3D.endGame();
        }
    }

    // Show time, balls and score
    Item {
        width: parent.width
        height: 60 * px
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 20 * px
            font.pixelSize: 26 * px
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            text: currentTime.toFixed(2)
        }
        Image {
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 1 * px
            anchors.right: ballCountText.left
            anchors.rightMargin: 8 * px
            width: 26 * px
            height: width
            mipmap: true
            source: "images/ball_icon.png"
        }
        Text {
            id: ballCountText
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 20 * px
            font.pixelSize: 26 * px
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            text: currentBalls
        }
        Text {
            anchors.centerIn: parent
            font.pixelSize: 36 * px
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            text: score
        }
    }

    // Game logo
    Image {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.6, sourceSize.width)
        height: width * 0.6
        fillMode: Image.PreserveAspectFit
        source: "images/quickball.png"
        opacity: !gameOn
        scale: 2.0 - opacity
        Behavior on opacity {
            NumberAnimation {
                duration: 400
                easing.type: Easing.InOutQuad
            }
        }
    }

    // Show bonus and total score when the game ends
    Item {
        property bool show: playingStarted && !gameOn

        anchors.centerIn: parent
        anchors.verticalCenterOffset: -200 * px
        onShowChanged: {
            if (show) {
                showScoreAnimation.start();
            } else {
                showScoreAnimation.stop();
                timeBonusText.opacity = 0;
                ballsBonusText.opacity = 0;
                totalScoreText.opacity = 0;
            }
        }

        SequentialAnimation {
            id: showScoreAnimation
            NumberAnimation {
                target: timeBonusText
                property: "opacity"
                to: 1
                duration: 1000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: ballsBonusText
                property: "opacity"
                to: 1
                duration: 1000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: totalScoreText
                property: "opacity"
                to: 1
                duration: 1000
                easing.type: Easing.InOutQuad
            }
        }

        Text {
            id: timeBonusText
            anchors.horizontalCenter: parent.horizontalCenter
            y: opacity * 60 * px
            font.pixelSize: 26 * px
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            textFormat: Text.StyledText
            text: qsTr("TIME BONUS <b>%1</b>").arg(timeBonus)
            opacity: 0
        }
        Text {
            id: ballsBonusText
            anchors.horizontalCenter: parent.horizontalCenter
            y: timeBonusText.y + opacity * 40 * px
            font.pixelSize: 26 * px
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            textFormat: Text.StyledText
            text: qsTr("BALLS BONUS <b>%1</b>").arg(ballsBonus)
            opacity: 0
        }
        Text {
            id: totalScoreText
            anchors.horizontalCenter: parent.horizontalCenter
            y: ballsBonusText.y + opacity * 60 * px
            font.pixelSize: 66 * px
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            textFormat: Text.StyledText
            text: qsTr("SCORE <b>%1</b>").arg(score + timeBonus + ballsBonus)
            opacity: 0
        }
    }

    Rectangle {
        id: startButton
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40 * px
        width: 140 * px
        height: 60 * px
        visible: !gameOn
        color: "#ffffff"
        border.color: "#000000"
        border.width: 2 * px
        radius: height / 2
        Text {
            anchors.centerIn: parent
            font.pixelSize: 26 * px
            text: qsTr("START")
        }
        MouseArea {
            anchors.fill: parent
            anchors.margins: -10 * px
            onClicked: {
                view3D.startGame();
            }
        }
    }
}
