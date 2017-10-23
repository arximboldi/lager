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

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <functional>
#include <variant>

extern "C" {
#include <ncurses.h>
}

namespace ncurses {

struct coord
{
    int row = {};
    int col = {};
};

using key_code = std::tuple<int, wint_t>;

struct key_event { key_code key; };
struct resize_event { coord size; };

using event = std::variant<
    key_event,
    resize_event>;

struct terminal
{
    using event_handler = std::function<void(event)>;

    terminal(boost::asio::io_service& serv);

    coord size();

    void start(event_handler ev);
    void stop();

private:
    struct cleanup_fn
    {
        void operator() (_win_st* win) const;
    };

    void next_key_();
    void next_resize_();

    std::unique_ptr<_win_st, cleanup_fn> win_;
    boost::asio::posix::stream_descriptor input_;
    boost::asio::signal_set signal_;
    event_handler handler_;
};

} // namespace ncurses
