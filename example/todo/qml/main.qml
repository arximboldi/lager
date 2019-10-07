
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.11

import Lager.Example.Todo 1.0

ApplicationWindow {
    width: 540
    height: 960
    visible: true

    Model {
        id: theModel
    }

    Page {
        anchors.fill: parent
        anchors.margins: 12
        header: Label {
            padding: 10
            text: qsTr("TODOS")
            font.pixelSize: 24
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        ColumnLayout {
            anchors.fill: parent
            TextField {
                Layout.fillWidth: true
                placeholderText: qsTr("What do you wanna do today?")
                onAccepted: {
                    theModel.add(text)
                    theModel.commit()
                    text = ""
                }
                Component.onCompleted: forceActiveFocus()
                onFocusChanged: forceActiveFocus()
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: theModel.count
                delegate: RowLayout {
                    property Todo todo: theModel.todo(index)
                    opacity: todo.done ? 0.5 : 1
                    CheckBox {
                        checked: todo.done
                        onClicked: {
                            todo.done = !todo.done
                            theModel.commit()
                        }
                    }
                    Text {
                        text: todo.text
                        font.strikeout: todo.done
                    }
                }
            }
        }
    }
}
