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

#include "autopong.hpp"

namespace autopong {

namespace {

float segment_squared_distance(point l1p1, point l1p2, point l2p1, point l2p2)
{
    constexpr auto epsilon = 0.00000001;
    auto u                 = l1p2 - l1p1;
    auto v                 = l2p2 - l2p1;
    auto w                 = l1p1 - l2p1;
    auto a = dot(u, u), b = dot(u, v), c = dot(v, v), d = dot(u, w),
         e = dot(v, w);
    auto D = a * c - b * b;
    float sc, sN, sD = D;
    float tc, tN, tD = D;
    if (D < epsilon) {
        sN = 0.0;
        sD = 1.0;
        tN = e;
        tD = c;
    } else {
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0.0) {
            sN = 0.0;
            tN = e;
            tD = c;
        } else if (sN > sD) {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }
    if (tN < 0.0) {
        tN = 0.0;
        if (-d < 0.0)
            sN = 0.0;
        else if (-d > a)
            sN = sD;
        else {
            sN = -d;
            sD = a;
        }
    } else if (tN > tD) {
        tN = tD;
        if ((-d + b) < 0.0)
            sN = 0;
        else if ((-d + b) > a)
            sN = sD;
        else {
            sN = (-d + b);
            sD = a;
        }
    }
    sc      = std::abs(sN) < epsilon ? 0.0 : sN / sD;
    tc      = std::abs(tN) < epsilon ? 0.0 : tN / tD;
    auto dP = w + (u * sc) - (v * tc);
    return norm(dP);
}

} // namespace

model update(model g, action a)
{
    return std::visit(
        lager::visitor{
            [&](paddle_move_action a) {
                g.paddle_x =
                    std::max(0.f,
                             std::min((float) window_width - paddle_width,
                                      g.paddle_x + a.delta * paddle_sens));
                return g;
            },
            [&](tick_action a) {
                auto ball = g.ball + g.ball_v * a.delta;
                g.death_anim =
                    std::max(0.f, g.death_anim - a.delta * death_anim_speed);
                g.bounce_anim =
                    std::max(0.f, g.bounce_anim - a.delta * bounce_anim_speed);
                if ((x(g.ball_v) < 0 && x(ball) - ball_r <= padding) ||
                    (x(g.ball_v) > 0 &&
                     x(ball) + ball_r >= window_width - padding))
                    x(g.ball_v, -x(g.ball_v));
                if (y(g.ball_v) < 0 && y(ball) - ball_r <= padding)
                    y(g.ball_v, -y(g.ball_v));
                if (y(g.ball_v) > 0 &&
                    ball_r * ball_r >
                        segment_squared_distance(
                            g.ball,
                            ball,
                            point{g.paddle_x - ball_r, paddle_y},
                            point{g.paddle_x + paddle_width + ball_r,
                                  paddle_y})) {
                    y(g.ball_v, -y(g.ball_v));
                    g.ball_v *= ball_a;
                    g.score++;
                    g.bounce_anim = 1;
                } else if (y(g.ball_v) > 0 &&
                           y(ball) - ball_r >= window_height - padding) {
                    g.max_score  = std::max(g.max_score, g.score);
                    g.score      = 0;
                    g.ball_v     = ball_init_v;
                    g.ball       = {padding + std::rand() /
                                            static_cast<float>(RAND_MAX) *
                                            (window_width - padding * 4),
                              padding * 2};
                    g.death_anim = 1;
                } else
                    g.ball = ball;
                return g;
            }},
        a);
}

} // namespace autopong
