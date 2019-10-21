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

#include "qmodel.hpp"

namespace sn {

namespace {

QPoint as_qpoint(std::pair<int, int> p)
{
    auto [x, y] = p;
    return {x, y};
}

} // namespace

SnakeBody::SnakeBody(QObject* parent)
    : QAbstractListModel(parent)
{}

void SnakeBody::setModel(snake_model::body_t body)
{
    if (data_ != body) {
        beginResetModel();
        data_ = body;
        endResetModel();
    }
}

int SnakeBody::rowCount(const QModelIndex&) const { return data_.size(); }

QVariant SnakeBody::data(const QModelIndex& index, int) const
{
    if (index.row() < static_cast<int>(data_.size()))
        return as_qpoint(data_[index.row()]);
    return {};
}

Game::Game(context_t context)
    : context_{std::move(context)}
    , snakeBody_{new SnakeBody(this)}
{}

void Game::setModel(game_model game)
{
    snakeBody_->setModel(game.snake.body);

    auto newApplePosition = as_qpoint(game.apple_pos);
    if (applePosition_ != newApplePosition) {
        applePosition_ = newApplePosition;
        emit applePositionChanged(applePosition_);
    }

    if (over_ != game.over) {
        over_ = game.over;
        emit overChanged();
    }
}

Q_INVOKABLE void Game::left() { context_.dispatch(go_left{}); }

Q_INVOKABLE void Game::right() { context_.dispatch(go_right{}); }

Q_INVOKABLE void Game::up() { context_.dispatch(go_up{}); }

Q_INVOKABLE void Game::down() { context_.dispatch(go_down{}); }

Q_INVOKABLE void Game::reset() { context_.dispatch(sn::reset{}); }

Q_INVOKABLE void Game::tick() { context_.dispatch(sn::tick{}); }

} // namespace sn
