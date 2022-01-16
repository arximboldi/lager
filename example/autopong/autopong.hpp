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

#include <lager/extra/struct.hpp>
#include <lager/util.hpp>

#include <complex>
#include <variant>

namespace autopong {

using point = std::complex<float>;

inline float x(const point& p) { return p.real(); }
inline void x(point& p, float v) { return p.real(v); }
inline float y(const point& p) { return p.imag(); }
inline void y(point& p, float v) { return p.imag(v); }
inline float dot(const point& a, const point& b) { return x(conj(a) * b); }

constexpr auto window_width  = 800;
constexpr auto window_height = 600;
constexpr auto padding       = 20;
constexpr auto border        = 4;
constexpr auto ball_r        = 4;
constexpr auto ball_init_v   = point{0.2f, 0.2f};
constexpr auto ball_a        = 1.1f;
constexpr auto paddle_width  = 100;
constexpr auto paddle_height = 10;
constexpr auto paddle_y      = window_height - 2 * padding - paddle_height;
constexpr auto paddle_sens   = 0.5f;

constexpr auto bounce_anim_speed = 0.002f;
constexpr auto death_anim_speed  = 0.001f;

struct model
{
    int score         = 0;
    int max_score     = 0;
    point ball        = {window_width / 2, padding * 2};
    point ball_v      = ball_init_v;
    float paddle_x    = window_width / 2 - paddle_width / 2;
    float death_anim  = 0;
    float bounce_anim = 0;
};

struct paddle_move_action
{
    float delta;
};

struct tick_action
{
    float delta;
};

using action = std::variant<paddle_move_action, tick_action>;

model update(model g, action a);

} // namespace autopong

LAGER_STRUCT(autopong,
             model,
             score,
             max_score,
             ball,
             ball_v,
             paddle_x,
             death_anim,
             bounce_anim);
LAGER_STRUCT(autopong, paddle_move_action, delta);
LAGER_STRUCT(autopong, tick_action, delta);
