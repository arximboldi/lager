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

#include <SDL.h>
#include <SDL_ttf.h>

#include <lager/store.hpp>
#include <lager/event_loop/sdl.hpp>

#include <variant>
#include <string>

constexpr auto padding       = 20;
constexpr auto ball_size     = 10;
constexpr auto paddle_width  = 100;
constexpr auto paddle_height = 10;

struct game
{
    int score;
    int ball_x;
    int ball_y;
    int paddle_x;
};

struct paddle_move_action { int center_x; };
struct tick_action { float delta; };

using action = std::variant<
    paddle_move_action,
    tick_action>;

game update(game g, action a)
{
    return std::visit(lager::visitor{
            [&] (paddle_move_action a) {
                g.paddle_x = a.center_x - paddle_width / 2;
                return g;
            },
            [&] (tick_action a) {
                return g;
            }
        }, a);
}

std::string font_path()
{
    using namespace std::string_literals;
    return lager::resources_path() + "/SourceSansPro-Regular.ttf"s;
}

struct sdl_view
{
    int width              = 640;
    int height             = 480;
    SDL_Window* window     = SDL_CreateWindow("Lager Counter Example",
                                              SDL_WINDOWPOS_UNDEFINED,
                                              SDL_WINDOWPOS_UNDEFINED,
                                              width, height,
                                              SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    TTF_Font* font         = TTF_OpenFont(font_path().c_str(), 24);
};

void draw(sdl_view& v, game g)
{
    SDL_RenderClear(v.renderer);
    // render background
    {
        SDL_SetRenderDrawColor(v.renderer, 0, 0, 0, 255);
        auto rect = SDL_Rect{0, 0, v.width, v.height};
        SDL_RenderFillRect(v.renderer, &rect);
    }
    // render text
    {
        auto msg  = "score: " + std::to_string(g.score);
        auto surf = TTF_RenderText_Blended(v.font, msg.c_str(), {255, 255, 255, 255});
        auto text = SDL_CreateTextureFromSurface(v.renderer, surf);
        SDL_FreeSurface(surf);

        auto rect = SDL_Rect{padding, padding};
        SDL_QueryTexture(text, nullptr, nullptr, &rect.w, &rect.h);
        SDL_RenderCopy(v.renderer, text, nullptr, &rect);
    }
    // render paddle
    {
        SDL_SetRenderDrawColor(v.renderer, 255, 255, 255, 255);
        auto rect = SDL_Rect{g.paddle_x, v.height - padding - paddle_height,
                             paddle_width, paddle_height};
        SDL_RenderFillRect(v.renderer, &rect);
    }
    SDL_RenderPresent(v.renderer);
}

std::optional<action> intent(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_MOUSEMOTION:
        return paddle_move_action{event.motion.x};
    default:
        return std::nullopt;
    }
}

int main()
{
    using namespace std::placeholders;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_ShowCursor(false);

    auto view  = sdl_view{};
    auto loop  = lager::sdl_event_loop{};
    auto store = lager::make_store<action>(
        game{},
        update,
        std::bind(draw, view, _1),
        lager::with_sdl_event_loop{loop});

    loop.run(
        [&] (const SDL_Event& ev) {
            if (auto act = intent(ev))
                store.dispatch(*act);
            return ev.type != SDL_QUIT;
        },
        [&] (float delta) {
            store.dispatch(tick_action{delta});
            return true;
        });
}
