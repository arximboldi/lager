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

#include "../autopong.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <lager/debug/debugger.hpp>
#include <lager/debug/http_server.hpp>
#include <lager/event_loop/sdl.hpp>
#include <lager/resources_path.hpp>
#include <lager/store.hpp>

#include <cereal/types/complex.hpp>
#include <lager/extra/cereal/struct.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <variant>

constexpr auto font_size = 32;

std::string font_path()
{
    using namespace std::string_literals;
    return lager::resources_path() + "/SourceSansPro-Bold.ttf"s;
}

struct sdl_view
{
    SDL_Window* window     = SDL_CreateWindow("Lager Counter Example",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          autopong::window_width,
                                          autopong::window_height,
                                          SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    TTF_Font* font         = TTF_OpenFont(font_path().c_str(), font_size);
};

void draw(sdl_view& v, autopong::model g)
{
    using namespace autopong;

    SDL_RenderClear(v.renderer);
    // render background
    {
        SDL_SetRenderDrawColor(v.renderer, 0, 0, 0, 255);
        auto rect = SDL_Rect{0, 0, window_width, window_height};
        SDL_RenderFillRect(v.renderer, &rect);
    }
    // render text
    {
        auto c   = (std::uint8_t)(255 *
                                (1 - std::cos(g.death_anim * M_PI - M_PI / 2)));
        auto msg = std::to_string(g.score);
        auto surf =
            TTF_RenderText_Blended(v.font, msg.c_str(), {255, c, c, 255});
        auto text = SDL_CreateTextureFromSurface(v.renderer, surf);
        SDL_FreeSurface(surf);
        auto rect = SDL_Rect{2 * padding, 2 * padding - font_size / 3};
        SDL_QueryTexture(text, nullptr, nullptr, &rect.w, &rect.h);
        SDL_RenderCopy(v.renderer, text, nullptr, &rect);
        if (g.max_score) {
            auto msg = std::to_string(g.max_score);
            auto surf =
                TTF_RenderText_Blended(v.font, msg.c_str(), {255, c, c, 255});
            auto text = SDL_CreateTextureFromSurface(v.renderer, surf);
            SDL_FreeSurface(surf);
            auto rect = SDL_Rect{2 * padding, 2 * padding - font_size / 3};
            SDL_QueryTexture(text, nullptr, nullptr, &rect.w, &rect.h);
            rect.x = window_width - rect.x - rect.w;
            SDL_RenderCopy(v.renderer, text, nullptr, &rect);
        }
    }
    // render border
    {
        auto c = (std::uint8_t)(255 *
                                (1 - std::cos(g.death_anim * M_PI - M_PI / 2)));
        SDL_SetRenderDrawColor(v.renderer, 255, c, c, 255);
        auto rect = SDL_Rect{padding - border,
                             padding - border,
                             window_width - 2 * padding + 2 * border,
                             window_height - 2 * padding + 2 * border};
        for (auto i = 0; i < border;
             ++i, ++rect.x, ++rect.y, ----rect.w, ----rect.h)
            SDL_RenderDrawRect(v.renderer, &rect);
    }
    // render game
    {
        auto rect = SDL_Rect{
            padding,
            padding,
            window_width - 2 * padding,
            window_height - 2 * padding,
        };
        auto c =
            (std::uint8_t)(255 * (1. - std::sin(g.bounce_anim * M_PI / 2)));
        SDL_RenderSetClipRect(v.renderer, &rect);
        SDL_SetRenderDrawColor(v.renderer, 255, c, c, 255);
        // paddle
        {
            auto rect = SDL_Rect{
                (int) g.paddle_x, paddle_y, paddle_width, paddle_height};
            SDL_RenderFillRect(v.renderer, &rect);
        }
        // ball
        {
            auto rect = SDL_Rect{(int) x(g.ball) - ball_r,
                                 (int) y(g.ball) - ball_r,
                                 ball_r * 2,
                                 ball_r * 2};
            SDL_RenderFillRect(v.renderer, &rect);
        }
        SDL_RenderSetClipRect(v.renderer, nullptr);
    }
    SDL_RenderPresent(v.renderer);
}

std::optional<autopong::action> intent(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_MOUSEMOTION:
        return autopong::paddle_move_action{(float) event.motion.xrel};
    default:
        return std::nullopt;
    }
}

int main(int argc, const char** argv)
{
    using namespace std::placeholders;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_SetRelativeMouseMode(SDL_TRUE);

#ifdef DEBUGGER
    auto debugger =
        lager::http_debug_server{argc, argv, 8080, lager::resources_path()};
#endif
    auto view = sdl_view{};
    auto loop = lager::sdl_event_loop{};
    auto store =
        lager::make_store<autopong::action>(autopong::model{},
                                            lager::with_sdl_event_loop{loop},
#ifdef DEBUGGER
                                            lager::with_debugger(debugger)
#else
                                            lager::identity
#endif
        );
    watch(store, [&](auto&& val) { draw(view, LAGER_FWD(val)); });
    loop.run(
        [&](const SDL_Event& ev) {
            if (auto act = intent(ev))
                store.dispatch(*act);
            return ev.type != SDL_QUIT;
        },
        [&](float delta) {
            store.dispatch(autopong::tick_action{delta});
            return true;
        });
}
