import QtQuick
import QtQuick3D

Item {
    width: 640
    height: 480

    function makeThirdReferToStandaloneSourceItem() {
        thirdTexture.sourceItem = standaloneSourceItem;
    }

    Item {
        id: standaloneSourceItem
        visible: false
        width: 100; height: 100
        Rectangle {
            anchors.fill: parent
            color: "red"
        }
    }

    View3D {
        anchors.fill: parent
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }
        PerspectiveCamera { z: 600 }
        Model {
            source: "#Sphere"
            x: -200
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    sourceItem: standaloneSourceItem
                }
            }
        }
        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    sourceItem: Rectangle {
                        width: 100; height: 100
                        color: "green"
                    }
                }
            }
        }
        Model {
            source: "#Sphere"
            x: 200
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    id: thirdTexture
                    sourceItem: Rectangle {
                        width: 100; height: 100
                        color: "blue"
                    }
                }
            }
        }
    }
}
