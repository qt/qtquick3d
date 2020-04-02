import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    property string description
    property int precision: 2
    property bool exponential: false
    property double expValue: exponential ? Math.pow(2.0, value) : value
    from: 0.0
    to: 1.0
    value: 0.5

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.verticalCenter
        anchors.topMargin: 6
        text: (parent.description.length == 0 ? "" : parent.description + ": ")
                   + parent.expValue.toFixed(precision);
        z: 10
    }
}
