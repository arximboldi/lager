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

#include "terminal.hpp"

#include <boost/asio/read.hpp>

#include <iostream>

using namespace std::placeholders;
using namespace std::string_literals;

namespace ncurses {

terminal::terminal(boost::asio::io_service& serv)
    : win_{[] {
        std::locale::global(std::locale(""));
        ::setlocale(LC_ALL, "");
        return ::initscr();
    }()}
    , input_{serv, ::dup(STDIN_FILENO)}
    , signal_{serv, SIGWINCH}
{
    if (win_.get() != ::stdscr)
        throw std::runtime_error{"error while initializing ncurses"};

    ::raw();
    ::noecho();
    ::keypad(stdscr, true);
    ::nodelay(stdscr, true);

    ::start_color();
    ::use_default_colors();
}

coord terminal::size()
{
    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    return {maxrow, maxcol};
}

void terminal::start(event_handler ev)
{
    assert(!handler_);
    handler_ = std::move(ev);
    next_key_();
    next_resize_();
}

void terminal::stop()
{
    input_.cancel();
    signal_.cancel();
    handler_ = {};
}

void terminal::next_resize_()
{
    signal_.async_wait([this](auto ec, auto) {
        if (!ec) {
            next_resize_();
            auto ws = ::winsize{};
            if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
                ::perror("TIOCGWINSZ");
            else {
                ::resizeterm(ws.ws_row, ws.ws_col);
                handler_(resize_event{{ws.ws_row, ws.ws_col}});
            }
        }
    });
}

void terminal::next_key_()
{
    using namespace boost::asio;
    input_.async_read_some(null_buffers(), [&](auto ec, auto) {
        if (!ec) {
            auto key = wint_t{};
            auto res = int{};
            while (ERR != (res = ::wget_wch(win_.get(), &key))) {
                next_key_();
                handler_(key_event{{res, key}});
            }
        }
    });
}

void terminal::cleanup_fn::operator()(WINDOW* win) const
{
    if (win) {
        // consume all remaining characters from the terminal so they
        // don't leak in the bash prompt after quitting, then restore
        // the terminal state
        auto key = wint_t{};
        while (::get_wch(&key) != ERR)
            ;
        ::endwin();
    }
}

} // namespace ncurses
