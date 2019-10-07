//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include "../todo.hpp"

#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>
#include <lager/state.hpp>

#include <QApplication>
#include <QObject>
#include <QQmlApplicationEngine>

class Todo : public QObject
{
    Q_OBJECT

public:
    Todo(lager::cursor<todo> data)
        : LAGER_QT(done){data[&todo::done]}
        , LAGER_QT(text){data[&todo::text].xf(
              zug::map([](auto&& x) { return QString::fromStdString(x); }),
              zug::map([](auto&& x) { return x.toStdString(); }))}
    {}

    LAGER_QT_CURSOR(bool, done);
    LAGER_QT_CURSOR(QString, text);
};

class Model : public QObject
{
    Q_OBJECT

    lager::state<model> state_;

public:
    Model()
        : LAGER_QT(count){state_.xf(zug::map(
              [](auto&& x) { return static_cast<int>(x.todos.size()); }))}
    {}

    Q_INVOKABLE Todo* todo(int index)
    {
        return new Todo{state_[&model::todos][index]};
    }

    LAGER_QT_READER(int, count);

    Q_INVOKABLE void add(QString text)
    {
        state_.update([&](auto x) {
            x.todos.push_back({false, text.toStdString()});
            return x;
        });
    }

    Q_INVOKABLE void commit() { lager::commit(state_); }
};

#include "main.moc"

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    QQmlApplicationEngine engine;

    qmlRegisterType<Model>("Lager.Example.Todo", 1, 0, "Model");
    qmlRegisterUncreatableType<Todo>(
        "Lager.Example.Todo", 1, 0, "Todo", "uncreatable");

    engine.load(LAGER_TODO_QML_DIR "/main.qml");

    return app.exec();
}
