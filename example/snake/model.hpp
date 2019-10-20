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

#include <boost/variant/variant.hpp>

#include <cstdlib>
#include <random>
#include <utility>
#include <vector>

namespace sn {

struct go_left
{};
struct go_right
{};
struct go_up
{};
struct go_down
{};
struct reset
{};
struct tick
{};
using action_t = boost::variant<go_left, go_right, go_up, go_down, reset, tick>;

using point_t = std::pair<int, int>;

inline int x(point_t p) { return p.first; }

inline int y(point_t p) { return p.second; }

enum class direction
{
    left,
    up,
    right,
    down
};

struct snake_model
{
    using body_t = std::vector<point_t>;
    body_t body{};
    direction dir{};
};

struct game_model
{
    snake_model snake;
    point_t apple_pos{};
    bool over;

    static constexpr int width{25};
    static constexpr int height{25};
};

struct app_model
{
    std::mt19937 gen;
    std::uniform_int_distribution<> dist;

    game_model game;
};

template <typename Func>
inline point_t random_apple_pos(Func&& random)
{
    return {random(), random()};
}

app_model make_initial(int seed);

app_model update(app_model m, action_t action);

} // namespace sn
