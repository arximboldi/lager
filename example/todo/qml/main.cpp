
#include "../todo.hpp"

#include <QApplication>
#include <QObject>
#include <QQmlApplicationEngine>

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    QQmlApplicationEngine engine;
    engine.load(LAGER_TODO_QML_DIR "/main.qml");
    return app.exec();
}
