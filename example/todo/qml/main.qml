
import QtQuick 2.6
import QtQuick.Controls 2.1

ApplicationWindow {
    width: 540
    height: 960
    visible: true
    Page {
        anchors.fill: parent
        header: Label {
            padding: 10
            text: qsTr("TODOS")
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
