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

#include <catch.hpp>

#include <QCoreApplication>

#include <lager/event_loop/qt.hpp>
#include <lager/store.hpp>

#include "loop.hpp"

namespace {
void run_test(loop::context ctx, lager::reader<loop::model> reader) {
    ctx.dispatch(loop::set_main_id_action{std::this_thread::get_id()});

    // Process the set_main_id_action
    QCoreApplication::processEvents();
    // Perform model update
    QCoreApplication::processEvents();

    auto main_thread_id = std::this_thread::get_id();
    CHECK(reader->main_id == main_thread_id);
    CHECK(reader->worker_id == std::thread::id{});

    ctx.dispatch(loop::request_async_work_action{});

    // Process the request_async_work_action
    QCoreApplication::processEvents();
    // Process the effect - launch the worker thread
    QCoreApplication::processEvents();

    // Allow for a context switch for the worker to do it's job
    QThread::msleep(100);

    // Process the set_worker_id_action dispatched by the asynchronous worker
    QCoreApplication::processEvents();
    // Perform model update
    QCoreApplication::processEvents();

    CHECK(reader->main_id == main_thread_id);
    CHECK(reader->worker_id != std::thread::id{});
    CHECK(reader->worker_id != main_thread_id);
}
}

TEST_CASE("global thread pool")
{
    int argc = 0;
    QCoreApplication app{argc, nullptr};
    auto store = lager::make_store<loop::action>(
        loop::model{},
        loop::update,
        lager::with_qt_event_loop{app});
    run_test(store, store);
}

TEST_CASE("manualy defined thread pool")
{
    int argc = 0;
    QCoreApplication app{argc, nullptr};
    QThreadPool thread_pool;
    auto store = lager::make_store<loop::action>(
        loop::model{},
        loop::update,
        lager::with_qt_event_loop{app, thread_pool});
    run_test(store, store);
}
