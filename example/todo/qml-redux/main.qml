
import QtQuick 2.6
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.11

import QtQuick.Dialogs 1.3

import Lager.Example.Todo 1.0 as Todo

ApplicationWindow {
    width: 540
    height: 960
    visible: true

    Material.theme: Material.Dark

    Todo.App {
        id: app
        onError: {
            errorDialog.text = text;
            errorDialog.open()
        }
    }

    MessageDialog {
        id: errorDialog
    }

    FileDialog {
        id: loadFileChooser
        defaultSuffix: "todo"
        nameFilters: ["Todo files (*.todo)"]
        selectExisting: true
        onAccepted: {
            app.load(loadFileChooser.fileUrl)
        }
    }

    FileDialog {
        id: saveFileChooser
        defaultSuffix: "todo"
        nameFilters: ["Todo files (*.todo)"]
        selectExisting: false
        onAccepted: {
            app.save(saveFileChooser.fileUrl)
        }
    }

    Action {
        id: loadAction
        text: qsTr("&Load")
        shortcut: StandardKey.Load
        onTriggered: loadFileChooser.open()
    }

    Action {
        id: saveAsAction
        text: qsTr("Save as...")
        shortcut: StandardKey.SaveAs
        onTriggered: saveFileChooser.open()
    }

    Action {
        id: saveAction
        text: qsTr("&Save")
        shortcut: StandardKey.Save
        onTriggered: {
            if (app.path)
                app.save(app.path)
            else
                saveAsAction.trigger()
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 12
            ToolButton {
                text: qsTr("Load")
                action: loadAction
            }
            Label {
                text: app.name
                font.bold: true
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
            ToolButton {
                action: saveAsAction
            }
            ToolButton {
                action: saveAction
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
                app.model.add(text)
                text = ""
            }
            Component.onCompleted: forceActiveFocus()
            onFocusChanged: forceActiveFocus()
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: app.model.count
            delegate: MouseArea {
                id: mouseArea
                readonly property Todo.Item todo: app.model.item(index)
                anchors.left: parent.left
                anchors.right: parent.right
                implicitHeight: layout.implicitHeight
                hoverEnabled: true
                onClicked: todo.toggle()
                RowLayout {
                    id: layout
                    anchors.fill: parent
                    opacity: todo.done ? 0.5 : 1
                    CheckBox {
                        checked: todo.done
                        onClicked: {
                            todo.done = !todo.done
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
                        onClicked: todo.remove()
                    }
                }
            }
        }
    }
}
