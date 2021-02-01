//
// lager - library for functional interactive c++ programs
// Copyright (C) 2019 Carl Bussey
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include "../model.hpp"
#include "qmodel.hpp"

#include <lager/event_loop/qt.hpp>
#include <lager/store.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <random>
#include <utility>

using namespace sn;

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    QQmlApplicationEngine engine;

    std::random_device rd;
    auto initial_state = make_initial(rd());
    auto store         = lager::make_store<action_t>(std::move(initial_state),
                                             lager::with_qt_event_loop{app});

    Game game{store};
    watch(store, [&](auto&& state) { game.setModel(state.game); });
    game.setModel(store.get().game);

    auto* qmlContext = engine.rootContext();
    qmlContext->setContextProperty(QStringLiteral("game"), &game);

    qRegisterMetaType<SnakeBody*>("SnakeBody*");

    engine.load(LAGER_SNAKE_QML_DIR "/main.qml");

    return app.exec();
}
