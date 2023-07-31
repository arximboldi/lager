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

#include <catch2/catch.hpp>

#include <QCoreApplication>

#include <lager/event_loop/qt.hpp>
#include <lager/store.hpp>

#include <thread>

namespace {

namespace loop {

struct model
{
    std::thread::id main_id;
    std::thread::id worker_id;
};

struct set_main_id_action
{
    std::thread::id main_id;
};

struct set_worker_id_action
{
    std::thread::id worker_id;
};
struct request_async_work_action
{};
using action = std::variant<set_main_id_action,
                            set_worker_id_action,
                            request_async_work_action>;

using effect  = lager::effect<action>;
using context = lager::context<action>;

inline std::pair<model, effect> update(model m, action action)
{
    return lager::match(action)(
        [&](set_main_id_action a) {
            m.main_id = a.main_id;
            return std::make_pair(m, effect{lager::noop});
        },
        [&](set_worker_id_action a) {
            m.worker_id = a.worker_id;
            return std::make_pair(m, effect{lager::noop});
        },
        [&](request_async_work_action) {
            effect async_work_effect = [](auto&& ctx) {
                ctx.loop().async([ctx]() {
                    ctx.dispatch(
                        set_worker_id_action{std::this_thread::get_id()});
                });
            };
            return std::make_pair(m, async_work_effect);
        });
}

} // namespace loop

void run_test(loop::context ctx, lager::reader<loop::model> reader)
{
    ctx.dispatch(loop::set_main_id_action{std::this_thread::get_id()});

    QCoreApplication::processEvents(); // run set_main_id_action
    QCoreApplication::processEvents(); // update model

    auto main_thread_id = std::this_thread::get_id();
    CHECK(reader->main_id == main_thread_id);
    CHECK(reader->worker_id == std::thread::id{});

    ctx.dispatch(loop::request_async_work_action{});

    QCoreApplication::processEvents(); // run request_async_work_action
    QCoreApplication::processEvents(); // run effect

    // allow for a context switch for the worker to do it's job
    QThread::msleep(100);

    QCoreApplication::processEvents(); // run set_worker_id_action
    QCoreApplication::processEvents(); // update model

    CHECK(reader->main_id == main_thread_id);
    CHECK(reader->worker_id != std::thread::id{});
    CHECK(reader->worker_id != main_thread_id);
}

} // namespace

TEST_CASE("global thread pool")
{
    int argc = 0;
    QCoreApplication app{argc, nullptr};
    auto store = lager::make_store<loop::action>(
        loop::model{}, lager::with_qt_event_loop{app});
    run_test(store, store);
}

TEST_CASE("manualy defined thread pool")
{
    int argc = 0;
    QCoreApplication app{argc, nullptr};
    QThreadPool thread_pool;
    auto store = lager::make_store<loop::action>(
        loop::model{}, lager::with_qt_event_loop{app, thread_pool});
    run_test(store, store);
}
