//
// lager - library for functional interactive c++ programs
//
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lager is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lager.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../counter.hpp"
#include "terminal.hpp"

#include <lager/store.hpp>
#include <lager/event_loop/boost_asio.hpp>
#include <lager/debug/http_server.hpp>
#include <lager/debug/enable.hpp>

void draw(counter::model c)
{
    ::clear();
    ::move(2, 4);
    ::printw("current value: %d", c.value);
    ::curs_set(false);
    ::refresh();
}

int main(int argc, const char** argv)
{
    auto serv  = boost::asio::io_service{};
    auto term  = ncurses::terminal{serv};

#ifdef DEBUGGER
    auto debugger = lager::http_debug_server{argc, argv, 8080};
#endif
#ifdef META_DEBUGGER
    auto meta_debugger = lager::http_debug_server{argc, argv, 8081};
#endif
    auto store = lager::make_store<counter::action>(
        counter::model{},
        counter::update,
        draw,
        lager::boost_asio_event_loop{serv},
        lager::comp(
#ifdef DEBUGGER
            lager::enable_debug(debugger),
#endif
#ifdef META_DEBUGGER
            lager::enable_debug(meta_debugger),
#endif
            lager::identity
            ));

    term.start([&] (auto ev) {
        std::visit(lager::visitor {
                [&] (ncurses::key_event ev) {
                    if (ev.key == ncurses::key_code{KEY_CODE_YES, KEY_UP})
                        store.dispatch(counter::increment_action{});
                    else if (ev.key == ncurses::key_code{KEY_CODE_YES, KEY_DOWN})
                        store.dispatch(counter::decrement_action{});
                    else if (ev.key == ncurses::key_code{OK, ' '})
                        store.dispatch(counter::reset_action{});
                    else if (ev.key == ncurses::key_code{OK, 'q'} ||
                             ev.key == ncurses::key_code{OK, '[' - '@'}) // esc
                        term.stop();
                },
                [&] (auto&&) {}
            }, ev);
    });

    serv.run();
}
