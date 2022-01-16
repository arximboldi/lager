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

#include "../model.hpp"

#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>
#include <lager/state.hpp>

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <iostream>

class Item : public QObject
{
    Q_OBJECT

public:
    Item(lager::cursor<todo::item> data)
        : LAGER_QT(done){data[&todo::item::done]}
        , LAGER_QT(text){data[&todo::item::text].xform(
              zug::map([](auto&& x) { return QString::fromStdString(x); }),
              zug::map([](auto&& x) { return x.toStdString(); }))}
    {}

    LAGER_QT_CURSOR(bool, done);
    LAGER_QT_CURSOR(QString, text);
};

class Model : public QObject
{
    Q_OBJECT

    lager::state<todo::model, lager::automatic_tag> state_;
    lager::state<QString, lager::automatic_tag> file_name_;

public:
    LAGER_QT_READER(QString, fileName);
    LAGER_QT_READER(QString, name);
    LAGER_QT_READER(int, count);

    Model()
        : LAGER_QT(fileName){file_name_}
        , LAGER_QT(name){file_name_.map(
              [](auto f) { return QFileInfo{f}.baseName(); })}
        , LAGER_QT(count){state_.xform(zug::map(
              [](auto&& x) { return static_cast<int>(x.todos.size()); }))}
    {}

    Q_INVOKABLE Item* todo(int index)
    {
        return new Item{
            state_[&todo::model::todos][index][lager::lenses::or_default]};
    }

    Q_INVOKABLE void add(QString text)
    {
        if (!text.isEmpty()) {
            state_.update([&](auto x) {
                x.todos = x.todos.push_front({false, text.toStdString()});
                return x;
            });
        }
    }

    Q_INVOKABLE void remove(int index)
    {
        state_.update([&](auto x) {
            x.todos = x.todos.erase(index);
            return x;
        });
    }

    Q_INVOKABLE bool save(QString fname)
    {
        try {
            auto fpath = QUrl{fname}.toLocalFile();
            if (QFileInfo{fname}.suffix() != "todo")
                fpath += ".todo";
            todo::save(fpath.toStdString(), *state_);
            file_name_.set(fname);
            return true;
        } catch (std::exception const& err) {
            std::cerr << "Exception thrown: " << err.what() << std::endl;
            return false;
        }
    }

    Q_INVOKABLE bool load(QString fname)
    {
        try {
            auto model = todo::load(QUrl{fname}.toLocalFile().toStdString());
            state_.set(model);
            file_name_.set(fname);
            return true;
        } catch (std::exception const& err) {
            std::cerr << "Exception thrown: " << err.what() << std::endl;
            return false;
        }
    }
};

#include "main.moc"

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    QQmlApplicationEngine engine;

    qmlRegisterType<Model>("Lager.Example.Todo", 1, 0, "Model");
    qmlRegisterUncreatableType<Item>("Lager.Example.Todo", 1, 0, "Item", "");

    QQuickStyle::setStyle("Material");

    engine.load(LAGER_TODO_QML_DIR "/main.qml");

    return app.exec();
}
