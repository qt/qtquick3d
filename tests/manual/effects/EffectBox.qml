import QtQuick 2.15
import QtQuick.Controls 2.15

CheckBox {
    onCheckedChanged: {
        parent.recalcEffects()
    }
    property var effect
}
