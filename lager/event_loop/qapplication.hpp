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

#include <QCoreApplication>
#include <QMetaObject>
#include <Qt>
#include <QtConcurrentRun>

#include <functional>
#include <utility>

namespace lager {

struct with_qapplication_event_loop
{
    std::reference_wrapper<QCoreApplication> app;

    template <typename Fn>
    void async(Fn&& fn)
    {
        QtConcurrent::run(std::forward<Fn>(fn));
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        QMetaObject::invokeMethod(
            &app.get(), std::forward<Fn>(fn), Qt::QueuedConnection);
    }

    void finish() { app.get().quit(); }

    void pause() {}
    void resume() {}
};

} // namespace lager
