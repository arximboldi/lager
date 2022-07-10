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

#include <lager/config.hpp>

#include <QObject>
#include <QThreadPool>
#include <QtConcurrent>

#include <functional>
#include <stdexcept>
#include <utility>

namespace lager {

struct with_qt_event_loop
{
    std::reference_wrapper<QObject> obj;
    std::reference_wrapper<QThreadPool> thread_pool =
        *QThreadPool::globalInstance();

    ~with_qt_event_loop() { thread_pool.get().waitForDone(); }

    template <typename Fn>
    void async(Fn&& fn)
    {
        QtConcurrent::run(&thread_pool.get(), std::forward<Fn>(fn));
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        QMetaObject::invokeMethod(
            &obj.get(), std::forward<Fn>(fn), Qt::QueuedConnection);
    }

    void finish() { QCoreApplication::instance()->quit(); }

    void pause() { LAGER_THROW(std::runtime_error{"not implemented!"}); }
    void resume() { LAGER_THROW(std::runtime_error{"not implemented!"}); }
};

} // namespace lager
