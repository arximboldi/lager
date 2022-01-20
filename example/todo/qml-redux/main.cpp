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

#include "../app.hpp"

#include <lager/cursor.hpp>
#include <lager/event_loop/qml.hpp>
#include <lager/extra/qt.hpp>
#include <lager/store.hpp>

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <iostream>

ZUG_INLINE_CONSTEXPR auto to_qstring = zug::comp([](auto&& f) {
    return [f](auto&& p) {
        return f(QString::fromStdString(ZUG_FWD(p)))(
            [&](auto&& x) { return ZUG_FWD(x).toStdString(); });
    };
});

class Item : public QObject
{
    Q_OBJECT

    lager::context<todo::item_action> ctx_;

public:
    Item(lager::context<todo::item_action> ctx, lager::reader<todo::item> data)
        : ctx_{std::move(ctx)}
        , LAGER_QT(done){data[&todo::item::done]}
        , LAGER_QT(text){data[&todo::item::text][to_qstring]}
    {}

    LAGER_QT_READER(bool, done);
    LAGER_QT_READER(QString, text);

    Q_INVOKABLE void toggle() { ctx_.dispatch(todo::toggle_item_action{}); }
    Q_INVOKABLE void remove() { ctx_.dispatch(todo::remove_item_action{}); }
};

class Model : public QObject
{
    Q_OBJECT

    lager::context<todo::model_action> ctx_;
    lager::reader<todo::model> data_;

public:
    Model(lager::context<todo::model_action> ctx,
          lager::reader<todo::model> data)
        : ctx_{std::move(ctx)}
        , data_{std::move(data)}
        , LAGER_QT(count){data_.map(
              [](auto&& x) { return static_cast<int>(x.todos.size()); })}
    {}

    LAGER_QT_READER(int, count);

    Q_INVOKABLE Item* item(int index)
    {
        if (index < 0)
            return nullptr;
        auto idx = static_cast<std::size_t>(index);
        return new Item{
            {ctx_,
             [idx](auto a) {
                 return std::pair{idx, a};
             }},
            data_[&todo::model::todos][index][lager::lenses::or_default]};
    }

    Q_INVOKABLE void add(QString text)
    {
        ctx_.dispatch(todo::add_todo_action{text.toStdString()});
    }
};

class App : public lager::event_loop_quick_item
{
    Q_OBJECT;

    using base_t = lager::event_loop_quick_item;

    todo::logger logger_{.error = [this](auto&& text) {
        Q_EMIT error(QString::fromStdString(text));
    }};

    lager::store<todo::app_action, todo::app> store_ =
        lager::make_store<todo::app_action>(
            todo::app{},
            lager::with_qml_event_loop{*this},
            lager::with_deps(std::ref(logger_)));

    Model model_{store_, store_[&todo::app::doc]};

public:
    LAGER_QT_READER(QString, path);
    LAGER_QT_READER(QString, name);

    App(QQuickItem* parent = nullptr)
        : base_t{parent}
        , LAGER_QT(path){store_[&todo::app::path].map(
              [](auto&& p) { return p.string(); })[to_qstring]}
        , LAGER_QT(name){LAGER_QT(path).map(
              [](auto f) { return QFileInfo{f}.baseName(); })}
    {}

    Q_PROPERTY(Model* model READ model CONSTANT)
    Model* model() { return &model_; }

    Q_INVOKABLE void save(QUrl fname)
    {
        store_.dispatch(todo::save_action{fname.toLocalFile().toStdString()});
    }

    Q_INVOKABLE void load(QUrl fname)
    {
        store_.dispatch(todo::load_action{fname.toLocalFile().toStdString()});
    }

    Q_SIGNAL void error(QString text);
};

#include "main.moc"

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    QQmlApplicationEngine engine;

    qmlRegisterType<App>("Lager.Example.Todo", 1, 0, "App");
    qmlRegisterUncreatableType<Item>("Lager.Example.Todo", 1, 0, "Item", "");
    qmlRegisterUncreatableType<Model>("Lager.Example.Todo", 1, 0, "Model", "");

    QQuickStyle::setStyle("Material");

    engine.load(LAGER_TODO_QML_DIR "/main.qml");

    return app.exec();
}
