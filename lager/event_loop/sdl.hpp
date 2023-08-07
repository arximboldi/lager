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

#include <lager/config.hpp>

#include <SDL2/SDL.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include <cassert>
#include <cstddef>
#include <cstdint>

#if __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace lager {

struct with_sdl_event_loop;

namespace detail {

struct constant_fps_step
{
    int frame_rate_;
    float ticks_per_frame_;
    float max_ticks_per_frame_;

    int ticks_       = SDL_GetTicks();
    int frame_count_ = 0;
    int last_ticks_  = 0;

    constant_fps_step(int rate = 60, int min_sim_rate = 15)
        : frame_rate_{rate}
        , ticks_per_frame_{1000.f / rate}
        , max_ticks_per_frame_{1000.f / min_sim_rate}
    {}

    float operator()()
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
        frame_count_++;
        return std::min((float) delta_ticks, max_ticks_per_frame_);
    }
};

} // namespace detail

struct sdl_event_loop
{
    using event_fn = std::function<void()>;

#if __EMSCRIPTEN__
    std::function<bool(const SDL_Event&)> current_handler;
    std::function<void(float)> current_tick;
    unsigned last_ticks = 0;

    void step()
    {
        auto event = SDL_Event{};
        last_ticks = SDL_GetTicks();
        while (SDL_PollEvent(&event)) {
            if (event.type == post_event_type_) {
                auto fnp = static_cast<event_fn*>(event.user.data1);
                (*fnp)();
                delete fnp;
            } else {
                current_handler(event);
            }
        }
        auto ticks = SDL_GetTicks();
        auto dt    = static_cast<float>(ticks - last_ticks);
        current_tick(dt);
        last_ticks = ticks;
    }

    template <typename Fn1, typename Fn2>
    void run(Fn1&& handler, Fn2&& tick, int fps = 0, bool infinite = true)
    {
        static bool guard = false;
        assert(!guard && "only one instance is allowed!");
        guard           = true;
        current_handler = std::forward<Fn1>(handler);
        current_tick    = std::forward<Fn2>(tick);
        emscripten_set_main_loop_arg(
            [](void* loop_) {
                auto loop = static_cast<sdl_event_loop*>(loop_);
                loop->step();
            },
            this,
            fps,
            infinite);
    }
#else  // !__EMSCRIPTEN__

    template <typename Fn>
    void run(Fn&& handler)
    {
        auto continue_ = true;
        while (continue_ && !done_) {
            auto event = SDL_Event{};
            if (SDL_WaitEvent(&event)) {
                if (event.type == post_event_type_) {
                    auto fnp = static_cast<event_fn*>(event.user.data1);
                    (*fnp)();
                    delete fnp;
                } else {
                    continue_ = handler(event);
                }
            }
        }
    }

    template <typename Fn1, typename Fn2>
    void run(Fn1&& handler, Fn2&& tick, int fps = 120, int min_sim_fps = 15)
    {
        auto continue_ = true;
        auto step      = detail::constant_fps_step{fps, min_sim_fps};
        while (continue_ && !done_) {
            auto event = SDL_Event{};
            while (continue_ && ((!paused_ && SDL_PollEvent(&event)) ||
                                 (paused_ && SDL_WaitEvent(&event)))) {
                if (event.type == post_event_type_) {
                    auto fnp = std::unique_ptr<event_fn>{
                        static_cast<event_fn*>(event.user.data1)};
                    (*fnp)();
                } else {
                    continue_ = continue_ && (paused_ || handler(event));
                }
            }
            continue_ = continue_ && (paused_ || tick(step()));
        }
    }
#endif // !__EMSCRIPTEN__

    void post(event_fn ev)
    {
#if !__EMSCRIPTEN__
        auto event = SDL_Event{};
        SDL_zero(event);
        event.type       = post_event_type_;
        event.user.data1 = new event_fn{std::move(ev)};
        SDL_PushEvent(&event);
#else
        ::emscripten_set_timeout(
            [](void* data) {
                auto fn =
                    std::unique_ptr<event_fn>{static_cast<event_fn*>(data)};
                (*fn)();
            },
            0,
            new event_fn{std::move(ev)});
#endif
    }

    void finish() { done_ = true; }
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }

private:
    friend with_sdl_event_loop;

    std::atomic<bool> done_{false};
    std::atomic<bool> paused_{false};
    std::uint32_t post_event_type_ = SDL_RegisterEvents(1);
}; // namespace lager

struct with_sdl_event_loop
{
    std::reference_wrapper<sdl_event_loop> loop;

    template <typename Fn>
    void async(Fn&& fn)
    {
        LAGER_THROW(std::logic_error{"not implemented!"});
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        loop.get().post(std::forward<Fn>(fn));
    }

    void finish() { loop.get().finish(); }
    void pause() { loop.get().pause(); }
    void resume() { loop.get().resume(); }
};

} // namespace lager
