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

#pragma once

#include "../model.hpp"

#include <lager/context.hpp>

#include <QAbstractListModel>
#include <QObject>
#include <QPoint>

namespace sn {

class SnakeBody : public QAbstractListModel
{
    Q_OBJECT

public:
    SnakeBody(QObject* parent);

    void setModel(snake_model::body_t body);

    int rowCount(const QModelIndex& = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int = Qt::DisplayRole) const override;

private:
    snake_model::body_t data_;
};

class Game : public QObject
{
    Q_OBJECT

public:
    using context_t = lager::context<action_t>;

    Game(context_t context);

    Q_PROPERTY(int width MEMBER width_ CONSTANT)
    Q_PROPERTY(int height MEMBER height_ CONSTANT)
    Q_PROPERTY(
        QPoint applePosition MEMBER applePosition_ NOTIFY applePositionChanged)
    Q_PROPERTY(bool over MEMBER over_ NOTIFY overChanged)

    Q_PROPERTY(SnakeBody* snake MEMBER snakeBody_ CONSTANT)

    Q_INVOKABLE void left();
    Q_INVOKABLE void right();
    Q_INVOKABLE void up();
    Q_INVOKABLE void down();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void tick();

    void setModel(game_model game);

signals:
    void applePositionChanged(QPoint);
    void overChanged();

private:
    context_t context_;

    SnakeBody* snakeBody_;

    static constexpr int width_{game_model::width};
    static constexpr int height_{game_model::height};
    QPoint applePosition_;
    bool over_{false};
};

} // namespace sn
