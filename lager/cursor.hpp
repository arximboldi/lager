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
#include <lager/detail/nodes.hpp>
#include <lager/reader.hpp>
#include <lager/watch.hpp>
#include <lager/writer.hpp>

#include <zug/meta/value_type.hpp>

namespace lager {

//! @defgroup cursors
//! @{

template <typename DerivT>
struct cursor_mixin
    : writer_mixin<DerivT>
    , reader_mixin<DerivT>
{
    using writer_mixin<DerivT>::operator[];
    using writer_mixin<DerivT>::make;
    using writer_mixin<DerivT>::zoom;
    using writer_mixin<DerivT>::xform;
    using reader_mixin<DerivT>::xform;

protected:
    ~cursor_mixin() = default;
};

template <typename NodeT>
class cursor_base
    : public cursor_mixin<cursor_base<NodeT>>
    , public xform_mixin<cursor_base<NodeT>>
    , public watchable_base<NodeT>
{
    friend class detail::access;

    using base_t = watchable_base<NodeT>;

public:
    using value_type = zug::meta::value_t<NodeT>;

    cursor_base() = default;

    template <typename T,
              std::enable_if_t<std::is_same_v<zug::meta::value_t<NodeT>,
                                              zug::meta::value_t<T>>,
                               int> = 0>
    cursor_base(cursor_base<T> x)
        : base_t{std::move(x)}
    {}

    template <typename NodeT2>
    cursor_base(std::shared_ptr<NodeT2> n)
        : base_t{std::move(n)}
    {}
};

/*!
 * Provides access to reading and writing values of type `T`.
 */
template <typename T>
class cursor : public cursor_base<detail::cursor_node<T>>
{
    using base_t = cursor_base<detail::cursor_node<T>>;

public:
    using base_t::base_t;
};

//! @}

} // namespace lager
