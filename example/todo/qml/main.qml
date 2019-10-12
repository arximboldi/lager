
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Layouts 1.11

import Lager.Example.Todo 1.0

ApplicationWindow {
    width: 540
    height: 960
    visible: true

    Material.theme: Material.Dark

    Model {
        id: theModel
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 12
            ToolButton {
                text: qsTr("Load")
                onClicked: stack.pop()
            }
            Label {
                text: "Title"
                font.bold: true
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: qsTr("Save as...")
                onClicked: menu.open()
            }
            ToolButton {
                text: qsTr("Save")
                onClicked: menu.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
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
            delegate: MouseArea {
                id: mouseArea
                property Todo todo: theModel.todo(index)
                anchors.left: parent.left
                anchors.right: parent.right
                implicitHeight: layout.implicitHeight
                hoverEnabled: true
                onClicked: {
                    todo.done = !todo.done
                    theModel.commit()
                }
                RowLayout {
                    id: layout
                    anchors.fill: parent
                    opacity: todo.done ? 0.5 : 1
                    CheckBox {
                        checked: todo.done
                        onClicked: {
                            todo.done = !todo.done
                            theModel.commit()
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: todo.text
                        font.strikeout: todo.done
                        font.bold: mouseArea.containsMouse
                    }
                    Button {
                        text: qsTr("Delete")
                        visible: mouseArea.containsMouse
                        palette {
                            button: "red"
                            text: "white"
                        }
                    }
                }
            }
        }
    }
}
