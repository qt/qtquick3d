import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    property string description
    property int precision: 2
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.verticalCenter
        anchors.bottomMargin: 16
        text: parent.description + ": " + parent.value.toFixed(precision);
        z: 10
    }
}
