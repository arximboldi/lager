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

#pragma once

#include <SDL.h>

#include <atomic>
#include <functional>

#include <cassert>
#include <cstddef>

namespace lager {

struct with_sdl_event_loop;

namespace detail {

struct constant_fps_step
{
    int frame_rate_;
    float ticks_per_frame_;

    int ticks_       = SDL_GetTicks();
    int frame_count_ = 0;
    int last_ticks_  = 0;

    constant_fps_step(int rate = 60)
        : frame_rate_{rate}
        , ticks_per_frame_{1000.f / rate}
    {}

    float operator() ()
    {
        auto current_ticks = SDL_GetTicks();
        auto delta_ticks   = current_ticks - ticks_;
        auto target_ticks  = last_ticks_ + ticks_per_frame_ * frame_count_;
        if (current_ticks <= target_ticks) {
            SDL_Delay(target_ticks - current_ticks);
        } else {
            frame_count_ = 0;
            last_ticks_  = SDL_GetTicks();
        }
        ticks_ = current_ticks;
        frame_count_ ++;
        return delta_ticks;
    }
};

} // detail

struct sdl_event_loop
{
    using event_fn = std::function<void()>;

    template <typename Fn>
    void run(Fn&& handler)
    {
        auto continue_ = true;
        while (continue_ && !done_) {
            auto event = SDL_Event{};
            if (SDL_WaitEvent(&event)) {
                if (event.type == post_event_type_) {
                    auto fnp = reinterpret_cast<event_fn*>(&event.user.data1);
                    (*fnp)();
                    fnp->~event_fn();
                } else {
                    continue_ = handler(event);
                }
            }
        }
    }

    template <typename Fn1, typename Fn2>
    void run(Fn1&& handler, Fn2&& tick, int fps = 120)
    {
        auto continue_ = true;
        auto step = detail::constant_fps_step{fps};
        while (continue_ && !done_) {
            auto event = SDL_Event{};
            while (continue_ &&
                   ((!paused_ && SDL_PollEvent(&event)) ||
                    (paused_ && SDL_WaitEvent(&event)))) {
                if (event.type == post_event_type_) {
                    auto fnp = reinterpret_cast<event_fn*>(&event.user.data1);
                    (*fnp)();
                    fnp->~event_fn();
                } else {
                    continue_ = continue_ && handler(event);
                }
            }
            continue_ = continue_ && (paused_ || tick(step()));
        }
    }

    void post(event_fn ev)
    {
        static_assert(sizeof(event_fn) <=
                      sizeof(SDL_Event) - offsetof(SDL_Event, user.data1),
                      "Ooops! A funciton does not fit in the SDL_Event");
        auto event = SDL_Event{};
        event.type = post_event_type_;
        auto region = static_cast<void*>(&event.user.data1);
        auto obj = new (region) event_fn{std::move(ev)};
        assert((void*) obj == (void*) &event.user.data1);
        SDL_PushEvent(&event);
    }

    void finish() { done_ = true; }
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }

private:
    friend with_sdl_event_loop;

    std::atomic<bool> done_ {false};
    std::atomic<bool> paused_ {false};
    std::uint32_t post_event_type_ = SDL_RegisterEvents(1);
};

struct with_sdl_event_loop
{
    std::reference_wrapper<sdl_event_loop> loop;

    template <typename Fn>
    void async(Fn&& fn)
    {
        throw std::logic_error{"not implemented!"};
    }

    template <typename Fn>
    void post(Fn&& fn) { loop.get().post(std::forward<Fn>(fn)); }

    void finish() { loop.get().finish(); }
    void pause() { loop.get().pause(); }
    void resume() { loop.get().resume(); }
 };

} // namespace lager
