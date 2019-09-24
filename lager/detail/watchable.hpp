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

#include <boost/signals2/signal.hpp>

namespace lager {

namespace detail {

template <typename ValueT>
class watchable
{
    using WatchersT =
        boost::signals2::signal<void(const ValueT&, const ValueT&)>;
    WatchersT watchers_;

public:
    watchable() = default;
    watchable(const watchable& other) noexcept {}
    watchable(watchable&& other) noexcept {}
    watchable& operator=(const watchable& other) noexcept { return *this; }
    watchable& operator=(watchable&& other) noexcept { return *this; }

    WatchersT& watchers() { return watchers_; }
};

} // namespace detail

} // namespace lager
