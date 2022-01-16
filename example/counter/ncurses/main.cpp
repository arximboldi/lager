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

#include "../counter.hpp"
#include "terminal.hpp"

#include <cereal/types/complex.hpp>

#include <lager/debug/debugger.hpp>
#include <lager/debug/http_server.hpp>
#include <lager/debug/tree_debugger.hpp>
#include <lager/event_loop/boost_asio.hpp>
#include <lager/resources_path.hpp>
#include <lager/store.hpp>

#include <zug/compose.hpp>

using namespace std::string_literals;

void draw(const counter::model& c)
{
    static const auto prelude      = "current value is "s;
    static const auto instructions = "  arrow up   -- decrease counter\n"
                                     "  arrow down -- increase counter\n"
                                     "  space bar  -- reset counter";
    auto message = prelude + std::to_string(c.value);
    auto max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    const auto border_y = 2, border_x = 4;
    auto width = message.size();
    auto pos_y = max_y / 2;
    auto pos_x = (max_x - message.size()) / 2;
    ::clear();
    auto color = c.value < -3  ? 3
                 : c.value > 3 ? 2
                               /* else */
                               : 1;
    ::attron(COLOR_PAIR(color));
    ::bkgd(COLOR_PAIR(color));
    attrset(A_NORMAL);
    mvprintw(max_y - 4, 0, instructions);
    attrset(A_NORMAL | A_REVERSE | A_BOLD);
    for (auto y = pos_y - border_y; y <= pos_y + border_y; ++y)
        mvhline(y, pos_x - border_x, ' ', width + 2 * border_x);
    mvaddnstr(pos_y, pos_x, message.data(), width);
    ::curs_set(false);
    ::refresh();
}

int main(int argc, const char** argv)
{
    auto serv = boost::asio::io_service{};
    auto term = ncurses::terminal{serv};
    ::init_pair(1, COLOR_WHITE, COLOR_GREEN);
    ::init_pair(2, COLOR_WHITE, COLOR_RED);
    ::init_pair(3, COLOR_WHITE, COLOR_BLUE);
#if defined(DEBUGGER) || defined(TREE_DEBUGGER)
    auto debugger =
        lager::http_debug_server{argc, argv, 8080, lager::resources_path()};
#endif
#ifdef META_DEBUGGER
    auto meta_debugger =
        lager::http_debug_server{argc, argv, 8081, lager::resources_path()};
#endif
    auto store = lager::make_store<counter::action>(
        counter::model{},
        lager::with_boost_asio_event_loop{serv.get_executor()},
        zug::comp(
#ifdef DEBUGGER
            lager::with_debugger(debugger),
#endif
#ifdef TREE_DEBUGGER
            lager::with_debugger<lager::tree_debugger>(debugger),
#endif
#ifdef META_DEBUGGER
            lager::with_debugger(meta_debugger),
#endif
            lager::identity));

    watch(store, [](auto&& val) { draw(unwrap(val)); });
    draw(unwrap(store.get()));

    term.start([&](auto ev) {
        std::visit(
            lager::visitor{
                [&](ncurses::key_event ev) {
                    if (ev.key == ncurses::key_code{KEY_CODE_YES, KEY_UP})
                        store.dispatch(counter::increment_action{});
                    else if (ev.key ==
                             ncurses::key_code{KEY_CODE_YES, KEY_DOWN})
                        store.dispatch(counter::decrement_action{});
                    else if (ev.key == ncurses::key_code{OK, ' '})
                        store.dispatch(counter::reset_action{});
                    else if (ev.key == ncurses::key_code{OK, 'q'} ||
                             ev.key == ncurses::key_code{OK, '[' - '@'}) // esc
                        term.stop();
                },
                [&](ncurses::resize_event) { draw(unwrap(store.get())); }},
            ev);
    });

    serv.run();
}
