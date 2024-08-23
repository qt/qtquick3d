import QtQuick
import QtQuick3D
import io.qt.tests.auto.Quick3DUpdateSpatialNode

View3D {
    width: 640
    height: 480
    anchors.fill: parent

    extensions: [ MyCustomExtension {
        }
    ]

    MyCustomResource {

    }

    MyCustomNode {

    }

    PerspectiveCamera {
    }
}
