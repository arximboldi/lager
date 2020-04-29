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

#include <lager/detail/access.hpp>
#include <lager/util.hpp>

namespace lager {

namespace detail {

template <typename RootCursorT>
void send_down_root(RootCursorT&& root)
{
    (void) detail::access::roots(std::forward<RootCursorT>(root))->send_down();
}

template <typename RootCursorT>
void notify_root(RootCursorT&& root)
{
    (void) detail::access::roots(std::forward<RootCursorT>(root))->notify();
}
} // namespace detail

/*!
 * Commit changes to a series of root cursors.  All values from the root cursors
 * are propagated before notifying any watchers.  This ensures that watchers
 * always see a consistent state of the world.
 */
template <typename... RootCursorTs>
void commit(RootCursorTs&&... roots)
{
    (detail::send_down_root(std::forward<RootCursorTs>(roots)), ...);
    (detail::notify_root(std::forward<RootCursorTs>(roots)), ...);
}

} // namespace lager
