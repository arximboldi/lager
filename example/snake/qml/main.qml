import QtQml 2.11
import QtQuick 2.6
import QtQuick.Controls 2.4

import QtQuick.Dialogs 1.3

ApplicationWindow {
    id: window

    width: 500
    height: 500
    visible: true

    property int modelScale: width / game.width

    readonly property int initial_timer_period: 100
    readonly property int timer_decrement: 10
    readonly property int min_timer_period: 10
    readonly property int timer_change_points: 5
    readonly property int initial_snake_size: 3

    Timer {
        interval: Math.max(initial_timer_period - timer_decrement *
                            Math.floor((snake.count - initial_snake_size) / timer_change_points)
                           , min_timer_period)
        repeat: true
        running: !game.over
        onTriggered: { game.tick() }
    }

    MessageDialog {
        id: overDialog
        text: qsTr("Game Over")
        onAccepted: {
            game.reset()
        }
    }

    Connections {
        target: game
        onOverChanged: {
            if (game.over)
                overDialog.open()
        }
    }

    Connections {
        target: game
        onApplePositionChanged: {
            apple.state == "other" ? apple.state = "" : apple.state = "other"
        }
    }

    Item {
        anchors.fill: parent

        Rectangle {
            id: background
            color: "black"
            anchors.fill: parent
        }

        focus: true
        Keys.onPressed: {
            if (event.key == Qt.Key_Left) {
                game.left()
                event.accepted = true
            }
            else if (event.key == Qt.Key_Right) {
                game.right()
                event.accepted = true
            }
            else if (event.key == Qt.Key_Up) {
                game.up()
                event.accepted = true
            }
            else if (event.key == Qt.Key_Down) {
                game.down()
                event.accepted = true
            }
        }

        Rectangle {
            id: apple
            width: modelScale
            height: modelScale
            x: game.applePosition.x * modelScale
            y: game.applePosition.y * modelScale
            color: "red"
            states: [
                State {
                    name: "other"
                    changes: [
                        PropertyChanges { target: apple; color: "green" }
                    ]
                }
            ]
        }

        Repeater {
            id: snake
            model: game.snake
            property color color: "dark green"
            delegate: Rectangle {
                width: modelScale
                height: modelScale
                x: model.display.x * modelScale
                y: model.display.y * modelScale
                color: snake.color
            }
        }
    }
}
