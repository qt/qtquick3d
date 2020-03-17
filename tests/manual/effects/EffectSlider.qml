import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    property string description
    property int precision: 2
    from: 0.0
    to: 1.0
    value: 0.5
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.verticalCenter
        anchors.bottomMargin: 16
        text: parent.description + ": " + parent.value.toFixed(precision);
        z: 10
    }
}
