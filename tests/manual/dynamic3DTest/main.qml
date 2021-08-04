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
import QtQuick.Window
import QtQuick3D
import QtQuick.Controls
import QtQuick.Layouts
import QtQml
import QtQuick3D.Helpers

import io.qt.tests.manual.dynamic3DTest

Window {
    width: 1280
    height: 720
    visible: true
    title: qsTr("Dynamic 3D Object Tester")


    Node {
        id: sceneRoot

        PerspectiveCamera {
            id: mainCamera
            z: 600
        }

        Model {
            id: mainSphere
            source: "#Sphere"
            materials: PrincipledMaterial {
                id: sphereMaterial
                baseColor: "blue"
            }

            function updateTexture() {
                if (objectSpawner.textures.length > 0)
                    sphereMaterial.baseColorMap = objectSpawner.textures[objectSpawner.textures.length - 1];
                else
                    sphereMaterial.baseColorMap = null

                if (objectSpawner.dynamicTextures.length > 0) {
                    sphereMaterial.emissiveFactor = Qt.vector3d(1, 1, 1)
                    sphereMaterial.emissiveMap = objectSpawner.dynamicTextures[objectSpawner.dynamicTextures.length - 1]
                } else {
                    sphereMaterial.emissiveMap = null
                    sphereMaterial.emissiveFactor = Qt.vector3d(0, 0, 0)
                }
            }

            NumberAnimation on y {
                from: 0
                to: 50
                duration: 10000
                running: true
                loops: -1
            }
        }

        Node {
            id: objectSpawner
            property real range: 300
            property var directionLights: []
            property var pointLights: []
            property var spotLights: []
            property var cameras: []
            property var models: []
            property var dynamicModels: []
            property var textures: []
            property var dynamicTextures: []

            property int directionLightsCount: 0
            property int pointLightsCount: 0
            property int spotLightsCount: 0
            property int camerasCount: 0
            property int modelsCount: 0
            property int dynamicModelsCount: 0
            property int texturesCount: 0
            property int dynamicTexturesCount: 0

            Component {
                id: directionalLight
                DirectionalLight {
                }
            }

            Component {
                id: pointLight
                PointLight {

                }
            }

            Component {
                id: spotLight
                SpotLight {

                }
            }

            Component {
                id: camera
                PerspectiveCamera {

                }
            }

            Component {
                id: model
                Model {
                    property alias color: material.diffuseColor
                    //source: "#Cube"
                    materials: DefaultMaterial {
                        id: material
                        diffuseColor: "red"
                    }
                }
            }

            Component {
                id: dynamicModel
                Model {
                    property alias color: material.diffuseColor
                    property alias lines: gridGeometry.horizontalLines
                    property alias step: gridGeometry.horizontalStep

                    geometry: GridGeometry {
                        id: gridGeometry
                        verticalLines: horizontalLines
                        verticalStep: horizontalStep
                    }

                    materials: DefaultMaterial {
                        id: material
                        diffuseColor: "red"
                    }
                }
            }

            Component {
                id: texture
                Texture {
                }
            }

            Component {
                id: dynamicTexture
                Texture {
                    property alias height: gradientTexture.height
                    property alias width: gradientTexture.width
                    property alias startColor: gradientTexture.startColor
                    property alias endColor: gradientTexture.endColor
                    textureData: GradientTexture {
                        id: gradientTexture

                    }
                }
            }

            function getRandomVector3d(range) {
                return Qt.vector3d((2 * Math.random() * range) - range,
                                   (2 * Math.random() * range) - range,
                                   (2 * Math.random() * range) - range)
            }

            function getRandomColor() {
                return Qt.rgba(Math.random(),
                               Math.random(),
                               Math.random(),
                               1.0)
            }

            function getRandomInt(min, max) {
                return Math.floor(Math.random() * max) + min;
            }

            function getMeshSource() {
                let sources = [
                        "random1.mesh",
                        "random2.mesh",
                        "random3.mesh",
                        "random4.mesh",
                        "#Cube",
                        "#Sphere",
                        "#Cone",
                        "#Cylinder"
                    ]
                let index = Math.floor(Math.random() * sources.length);
                return sources[index]
            }

            function getImageSource() {
                let sources = [
                        "noise1.jpg",
                        "noise2.jpg",
                        "noise3.jpg",
                        "noise4.jpg",
                        "noise5.jpg"
                    ]
                let index = Math.floor(Math.random() * sources.length);
                return sources[index]
            }

            function addDirectionalLight() {
                let position = getRandomVector3d(objectSpawner.range)
                let rotation = getRandomVector3d(360)
                let instance = directionalLight.createObject(objectSpawner, { "position": position, "eulerRotation": rotation})
                directionLights.push(instance);
                directionLightsCount++
            }
            function removeDirectionalLight() {
                if (directionLights.length > 0) {
                    let instance = directionLights.pop();
                    instance.destroy();
                    directionLightsCount--
                }
            }
            function addPointLight() {
                let position = getRandomVector3d(objectSpawner.range)
                let instance = pointLight.createObject(objectSpawner, { "position": position})
                pointLights.push(instance);
                pointLightsCount++
            }
            function removePointLight() {
                if (pointLights.length > 0) {
                    let instance = pointLights.pop();
                    instance.destroy();
                    pointLightsCount--
                }
            }
            function addSpotLight() {
                let position = getRandomVector3d(objectSpawner.range)
                let rotation = getRandomVector3d(360)
                let instance = spotLight.createObject(objectSpawner, { "position": position, "eulerRotation": rotation})
                spotLights.push(instance);
                spotLightsCount++

            }
            function removeSpotLight() {
                if (spotLights.length > 0) {
                    let instance = spotLights.pop();
                    instance.destroy();
                    spotLightsCount--
                }
            }
            function addCamera() {
                let position = getRandomVector3d(objectSpawner.range * 2)
                let rotation = getRandomVector3d(360)
                let instance = camera.createObject(objectSpawner, { "position": position, "eulerRotation": rotation})
                cameras.push(instance);
                instance.lookAt(Qt.vector3d(0, 0, 0))
                view2.updateCamera();
                camerasCount++
            }
            function removeCamera() {
                if (cameras.length > 0) {
                    let instance = cameras.pop();
                    instance.destroy();
                    camerasCount--
                }
                view2.updateCamera();
            }
            function addModel() {
                let position = getRandomVector3d(objectSpawner.range)
                let rotation = getRandomVector3d(360)
                let source = getMeshSource();
                let color = getRandomColor();
                let instance = model.createObject(objectSpawner, { "position": position, "eulerRotation": rotation, "color": color, "source": source})
                models.push(instance);
                modelsCount++
            }
            function removeModel() {
                if (models.length > 0) {
                    let instance = models.pop();
                    instance.destroy();
                    modelsCount--
                }
            }

            function addDynamicModel() {
                let position = getRandomVector3d(objectSpawner.range)
                let rotation = getRandomVector3d(360)
                let color = getRandomColor();
                let lines = getRandomInt(100, 1000);
                let steps = getRandomInt(1, 25);
                let instance = dynamicModel.createObject(objectSpawner, {"position": position, "eulerRotation": rotation, "color": color, "lines": lines, "step": steps})
                dynamicModels.push(instance)
                dynamicModelsCount++
            }

            function removeDynamicModel() {
                if (dynamicModels.length > 0) {
                    let instance = dynamicModels.pop();
                    instance.destroy();
                    dynamicModelsCount--
                }
            }

            function addTexture() {
                let source = getImageSource()
                let instance = texture.createObject(objectSpawner, {"source": source})
                textures.push(instance);
                texturesCount++
                mainSphere.updateTexture()
            }
            function removeTexture() {
                if (textures.length > 0) {
                    let instance = textures.pop();
                    instance.destroy();
                    texturesCount--
                    mainSphere.updateTexture()
                }
            }
            function changeTextures() {
                // reset the texture sources
                textures.forEach(texture => texture.source = getImageSource())
            }

            function addDynamicTexture() {
                let startColor = getRandomColor()
                let endColor = getRandomColor()
                let width = 2048
                let height = 2048
                let instance = dynamicTexture.createObject(objectSpawner, {"startColor": startColor, "endColor": endColor, "height": height, "width": width})
                dynamicTextures.push(instance);
                dynamicTexturesCount++
                mainSphere.updateTexture()
            }

            function removeDynamicTexture() {
                if (dynamicTextures.length > 0) {
                    let instance = dynamicTextures.pop();
                    instance.destroy();
                    dynamicTexturesCount--
                    mainSphere.updateTexture()
                }
            }

            function changeModels() {
                // reset the model sources
                models.forEach(model => model.source = getMeshSource())
            }
            function changeDynamicModels() {
                // reset the dynamic model sources
                if (dynamicModels.length == 0)
                    return

                var shuffleIndexs = Array.from({length: dynamicModels.length}, (x, i) => i);
                function shuffle(a) {
                    for (let i = a.length - 1; i > 0; i--) {
                        const j = Math.floor(Math.random() * (i + 1));
                        [a[i], a[j]] = [a[j], a[i]];
                    }
                    return a;
                }
                shuffleIndexs = shuffle(shuffleIndexs);
                var newSources = []
                shuffleIndexs.forEach(index => newSources.push(dynamicModels[index].geometry));
                for (var i = 0; i < dynamicModels.length; i++)
                    dynamicModels[i].geometry = newSources[i]
            }
        }

    }

    Item {
        id: viewsContainer
        anchors.top: parent.top
        anchors.right: controlsContainer.left
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        View3D {
            id: view1
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: parent.width * 0.5
            importScene: sceneRoot
            camera: mainCamera
            environment: SceneEnvironment {
                clearColor: "green"
                backgroundMode: SceneEnvironment.Color
            }
        }

        View3D {
            id: view2
            anchors.top: parent.top
            anchors.left: view1.right
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            importScene: sceneRoot
            environment: SceneEnvironment {
                clearColor: "pink"
                backgroundMode: SceneEnvironment.Color
            }

            function updateCamera() {
                if (objectSpawner.cameras.length > 0) {
                    var dynamicCamera = objectSpawner.cameras[objectSpawner.cameras.length - 1]
                    view2.camera = dynamicCamera
                } else {
                    view2.camera = mainCamera
                }
            }

            //camera: objectSpawner.cameras.length > 0 ? objectSpawner.cameras[objectSpawner.cameras.length - 1] : mainCamera
        }
    }

    Rectangle {
        id: controlsContainer
        width: 300
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: "grey"
        ColumnLayout {
            RowLayout {
                Label {
                    text: "Directional Light"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.directionLightsCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addDirectionalLight()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeDirectionalLight()
                    }
                }

            }
            RowLayout {
                Label {
                    text: "Point Light"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.pointLightsCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addPointLight()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removePointLight()
                    }
                }
            }
            RowLayout {
                Label {
                    text: "Spot Light"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.spotLightsCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addSpotLight()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeSpotLight()
                    }
                }
            }
            RowLayout {
                Label {
                    text: "Camera"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.camerasCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addCamera()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeCamera()
                    }
                }
            }
            RowLayout {
                Label {
                    text: "Model"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.modelsCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addModel()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeModel()
                    }
                }
                ToolButton {
                    text: "<->"
                    onClicked: {
                        objectSpawner.changeModels()
                    }
                }
            }
            RowLayout {
                Label {
                    text: "Dynamic Model"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.dynamicModelsCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addDynamicModel()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeDynamicModel()
                    }
                }
                ToolButton {
                    text: "<->"
                    onClicked: {
                        objectSpawner.changeDynamicModels()
                    }
                }
            }
            RowLayout {
                Label {
                    text: "Texture"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.texturesCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addTexture()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeTexture()
                    }
                }
                ToolButton {
                    text: "<->"
                    onClicked: {
                        objectSpawner.changeTextures()
                    }
                }
            }
            RowLayout {
                Label {
                    text: "Dynamic Texture"
                    color: "white"
                    Layout.fillWidth: true
                }
                Label {
                    text: objectSpawner.dynamicTexturesCount
                    color: "white"
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: "+"
                    onClicked: {
                        objectSpawner.addDynamicTexture()
                    }
                }
                ToolButton {
                    text: "-"
                    onClicked: {
                        objectSpawner.removeDynamicTexture()
                    }
                }
            }
        }
    }
}
