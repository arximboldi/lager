//
// lager - library for functional interactive c++ programs
// Copyright (C) 2019 Carl Bussey
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#pragma once

#include <lager/util.hpp>

#include <QCoreApplication>
#include <QMetaObject>
#include <QtConcurrent>
#include <QThreadPool>

#include <functional>
#include <utility>
#include <variant>

namespace lager {

struct with_qt_event_loop
{
    using qt_obj_ref_variant_t = std::variant<std::reference_wrapper<QObject>,
                                              std::reference_wrapper<QThreadPool>>;
    qt_obj_ref_variant_t obj_ref_variant;

    static QThreadPool *get_thread_pool_ptr(qt_obj_ref_variant_t obj_ref_variant) {
            return std::visit(lager::visitor{
                       [](std::reference_wrapper<QObject>) {
                           return QThreadPool::globalInstance();
                       },
                       [](std::reference_wrapper<QThreadPool> thread_pool) {
                           return &thread_pool.get();
                       }
                },
                obj_ref_variant);
        }

    ~with_qt_event_loop() {
        get_thread_pool_ptr(obj_ref_variant)->waitForDone();
    }

    template <typename Fn>
    void async(Fn&& fn)
    {
        auto thread_pool_ptr = get_thread_pool_ptr(obj_ref_variant);
        QtConcurrent::run(thread_pool_ptr, std::forward<Fn>(fn));
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        auto obj_ptr = get_thread_pool_ptr(obj_ref_variant);
        QMetaObject::invokeMethod(
            obj_ptr, std::forward<Fn>(fn), Qt::QueuedConnection);
    }

    void finish() { QCoreApplication::instance()->quit(); }

    void pause() { throw std::runtime_error{"not implemented!"}; }
    void resume() { throw std::runtime_error{"not implemented!"}; }
};

} // namespace lager
