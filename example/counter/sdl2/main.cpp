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

#include "../counter.hpp"
#include <lager/event_loop/sdl.hpp>
#include <lager/resources_path.hpp>
#include <lager/store.hpp>

#include <iostream>
#include <string>

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
                                          width,
                                          height,
                                          SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    TTF_Font* font         = TTF_OpenFont(font_path().c_str(), 24);
};

void draw(sdl_view& v, counter::model c)
{
    SDL_RenderClear(v.renderer);
    // render background
    {
        SDL_SetRenderDrawColor(v.renderer, 255, 96, 128, 255);
        auto rect = SDL_Rect{0, 0, v.width, v.height};
        SDL_RenderFillRect(v.renderer, &rect);
    }
    // render text
    {
        auto msg = "counter value is " + std::to_string(c.value);
        auto surf =
            TTF_RenderText_Blended(v.font, msg.c_str(), {255, 255, 255, 255});
        auto text = SDL_CreateTextureFromSurface(v.renderer, surf);
        SDL_FreeSurface(surf);

        auto rect = SDL_Rect{};
        SDL_QueryTexture(text, nullptr, nullptr, &rect.w, &rect.h);
        rect.x = v.width / 2 - rect.w / 2;
        rect.y = v.height / 2 - rect.h / 2;
        SDL_RenderCopy(v.renderer, text, nullptr, &rect);
    }
    SDL_RenderPresent(v.renderer);
}

std::optional<counter::action> intent(const SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_UP:
            return counter::increment_action{};
        case SDLK_DOWN:
            return counter::decrement_action{};
        case SDLK_SPACE:
            return counter::reset_action{};
        default:
            break;
        }
    }
    return std::nullopt;
}

int main()
{
    using namespace std::placeholders;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    auto view  = sdl_view{};
    auto loop  = lager::sdl_event_loop{};
    auto store = lager::make_store<counter::action>(
        counter::model{}, lager::with_sdl_event_loop{loop});

    watch(store, [&](auto&& val) { draw(view, val); });
    draw(view, store.get());

    loop.run([&](const SDL_Event& ev) {
        if (auto act = intent(ev))
            store.dispatch(*act);
        return ev.type != SDL_QUIT;
    });
}
