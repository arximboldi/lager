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

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/signal_set.hpp>

#include <functional>
#include <variant>

extern "C"
{
// this is required to avoid naming conflicts with boost::asio
#ifndef NCURSES_NOMACROS
#define NCURSES_NOMACROS 1
#endif
// this is required on Apple for ::get_wch
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1
#endif
#include <ncurses.h>
}

namespace ncurses {

struct coord
{
    int row = {};
    int col = {};
};

using key_code = std::tuple<int, wint_t>;

struct key_event
{
    key_code key;
};
struct resize_event
{
    coord size;
};
using event = std::variant<key_event, resize_event>;

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
        void operator()(_win_st* win) const;
    };

    void next_key_();
    void next_resize_();

    std::unique_ptr<_win_st, cleanup_fn> win_;
    boost::asio::posix::stream_descriptor input_;
    boost::asio::signal_set signal_;
    event_handler handler_;
};

} // namespace ncurses
